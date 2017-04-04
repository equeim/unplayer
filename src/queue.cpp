/*
 * Unplayer
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "queue.h"

#include <algorithm>

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QMimeDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlRecord>
#include <QtConcurrentRun>

#include <fileref.h>
#include <tag.h>

#include "libraryutils.h"
#include "playlistutils.h"
#include "tagutils.h"

namespace unplayer
{
    namespace
    {
        const QString dbConnectionName(QLatin1String("unplayer_queue"));
    }

    QueueTrack::QueueTrack(const QString& filePath,
                           const QString& title,
                           int duration,
                           const QString& artist,
                           const QString& album,
                           const QString& mediaArt)
        : filePath(filePath),
          title(title),
          duration(duration),
          artist(artist),
          album(album),
          mediaArt(mediaArt)
    {
    }

    Queue::Queue(QObject* parent)
        : QObject(parent),
          mCurrentIndex(-1),
          mShuffle(false),
          mRepeatMode(NoRepeat),
          mAddingTracks(false)
    {
        QObject::connect(LibraryUtils::instance(), &LibraryUtils::mediaArtChanged, this, [=]() {
            QSqlDatabase::database().transaction();
            for (const std::shared_ptr<QueueTrack>& track : mTracks) {
                QSqlQuery query;
                query.prepare(QStringLiteral("SELECT mediaArt FROM tracks WHERE filePath = ?"));
                query.addBindValue(track->filePath);
                if (query.exec()) {
                    if (query.next()) {
                        track->mediaArt = query.value(0).toString();
                    }
                } else {
                    qWarning() << "failed to get media art from database for track:" << track->filePath;
                }
            }
            QSqlDatabase::database().commit();
            emit mediaArtChanged();
        });
        QObject::connect(this, &Queue::currentTrackChanged, this, &Queue::mediaArtChanged);
    }

    const QList<std::shared_ptr<QueueTrack>>& Queue::tracks() const
    {
        return mTracks;
    }

    int Queue::currentIndex() const
    {
        return mCurrentIndex;
    }

    void Queue::setCurrentIndex(int index)
    {
        if (index != mCurrentIndex) {
            mCurrentIndex = index;
            emit currentIndexChanged();
        }
    }

    QString Queue::currentFilePath() const
    {
        if (mCurrentIndex >= 0 && mCurrentIndex < mTracks.size()) {
            return mTracks.at(mCurrentIndex)->filePath;
        }
        return QString();
    }

    QString Queue::currentTitle() const
    {
        if (mCurrentIndex >= 0 && mCurrentIndex < mTracks.size()) {
            return mTracks.at(mCurrentIndex)->title;
        }
        return QString();
    }

    QString Queue::currentArtist() const
    {
        if (mCurrentIndex >= 0 && mCurrentIndex < mTracks.size()) {
            return mTracks.at(mCurrentIndex)->artist;
        }
        return QString();
    }

    QString Queue::currentAlbum() const
    {
        if (mCurrentIndex >= 0 && mCurrentIndex < mTracks.size()) {
            return mTracks.at(mCurrentIndex)->album;
        }
        return QString();
    }

    QString Queue::currentMediaArt() const
    {
        if (mCurrentIndex >= 0 && mCurrentIndex < mTracks.size()) {
            return mTracks.at(mCurrentIndex)->mediaArt;
        }
        return QString();
    }

    bool Queue::isShuffle() const
    {
        return mShuffle;
    }

    void Queue::setShuffle(bool shuffle)
    {
        if (shuffle != mShuffle) {
            mShuffle = shuffle;
            emit shuffleChanged();
            if (!mShuffle) {
                resetNotPlayedTracks();
            }
        }
    }

    Queue::RepeatMode Queue::repeatMode() const
    {
        return mRepeatMode;
    }

    void Queue::changeRepeatMode()
    {
        switch (mRepeatMode) {
        case NoRepeat:
            mRepeatMode = RepeatAll;
            break;
        case RepeatAll:
            mRepeatMode = RepeatOne;
            if (mShuffle) {
                resetNotPlayedTracks();
            }
            break;
        case RepeatOne:
            mRepeatMode = NoRepeat;
        }
        emit repeatModeChanged();
    }

    bool Queue::isAddingTracks() const
    {
        return mAddingTracks;
    }

    void Queue::addTrack(const QString& track)
    {
        addTracks({track});
    }

    void Queue::addTracks(QStringList trackPaths, bool clearQueue, int setAsCurrent)
    {
        if (mAddingTracks) {
            return;
        }

        if (trackPaths.isEmpty()) {
            return;
        }

        mAddingTracks = true;
        emit addingTracksChanged();

        const QList<std::shared_ptr<QueueTrack>> oldTracks(mTracks);

        if (clearQueue) {
            clear();
        }

        auto future = QtConcurrent::run([trackPaths, oldTracks]() mutable {
            QList<std::shared_ptr<QueueTrack>> tracks;

            for (int i = 0, max = trackPaths.size(); i < max; ++i) {
                const QString& filePath = trackPaths.at(i);
                if (filePath.endsWith(QLatin1String(".pls"))) {
                    trackPaths.removeAt(i);
                    const QStringList playlistTracks(PlaylistUtils::getPlaylistTracks(filePath));
                    int trackIndex = i;
                    for (const QString& playlistTrack : playlistTracks) {
                        trackPaths.insert(trackIndex, playlistTrack);
                        ++trackIndex;
                    }
                    i += playlistTracks.size() - 1;
                    max += playlistTracks.size() - 1;
                }
            }

            {
                auto db = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"), dbConnectionName);
                db.setDatabaseName(LibraryUtils::databaseFilePath());
                if (!db.open()) {
                    qWarning() << "failed to open database" << db.lastError();
                }

                db.transaction();

                QHash<QString, QString> mediaArtDirectoriesHash;

                const QMimeDatabase mimeDb;

                for (const QString& filePath : trackPaths) {
                    bool found = false;
                    for (const std::shared_ptr<QueueTrack>& track : oldTracks) {
                        if (track->filePath == filePath) {
                            tracks.append(track);
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        continue;
                    }

                    const QFileInfo fileInfo(filePath);
                    if (!fileInfo.exists()) {
                        qWarning() << "file does not exists:" << filePath;
                        continue;
                    }

                    QString title;
                    QStringList artists;
                    QStringList albums;
                    int duration;
                    QString mediaArt;

                    bool getTags = true;

                    if (db.isOpen()) {
                        QSqlQuery query(db);
                        query.prepare(QStringLiteral("SELECT modificationTime, title, artist, album, duration, mediaArt FROM tracks WHERE filePath = ?"));
                        query.lastError();
                        query.addBindValue(filePath);
                        if (query.exec()) {
                            if (query.next()) {
                                if (query.value(0).toLongLong() == fileInfo.lastModified().toMSecsSinceEpoch()) {
                                    getTags = false;

                                    title = query.value(1).toString();
                                    duration = query.value(4).toInt();
                                    mediaArt = query.value(5).toString();

                                    query.previous();
                                    while (query.next()) {
                                        artists.append(query.value(2).toString());
                                        albums.append(query.value(3).toString());
                                    }
                                    artists.removeDuplicates();
                                    artists.removeAll(QString());
                                    albums.removeDuplicates();
                                    albums.removeAll(QString());
                                }
                            }
                        } else {
                            qWarning() << "failed to get track from database" << query.lastError();
                        }
                    }

                    if (getTags) {
                        const tagutils::Info info(tagutils::getTrackInfo(fileInfo, mimeDb.mimeTypeForFile(filePath).name()));
                        title = info.title;
                        artists = info.artists;
                        albums = info.albums;
                        duration = info.duration;
                        mediaArt = LibraryUtils::findMediaArtForDirectory(mediaArtDirectoriesHash, fileInfo.path());
                    }

                    QString artist;
                    if (artists.isEmpty()) {
                        artist = qApp->translate("unplayer", "Unknown artist");
                    } else {
                        artist = artists.join(QLatin1String(", "));
                    }

                    QString album;
                    if (albums.isEmpty()) {
                        album = qApp->translate("unplayer", "Unknown artist");
                    } else {
                        album = albums.join(QLatin1String(", "));
                    }

                    tracks.append(std::make_shared<QueueTrack>(filePath,
                                                               title,
                                                               duration,
                                                               artist,
                                                               album,
                                                               mediaArt));
                }

                db.commit();
            }
            QSqlDatabase::removeDatabase(dbConnectionName);

            return tracks;
        });

        using FutureWatcher = QFutureWatcher<QList<std::shared_ptr<QueueTrack>>>;
        auto watcher = new FutureWatcher(this);
        QObject::connect(watcher, &FutureWatcher::finished, this, [=]() {
            const int start = mTracks.size();
            const auto tracks = watcher->result();

            for (const auto track : tracks) {
                mTracks.append(track);
                mNotPlayedTracks.append(track);
            }
            emit tracksAdded(start);

            if (mCurrentIndex == -1 && !mTracks.isEmpty()) {
                if (setAsCurrent < 0 || setAsCurrent >= mTracks.size()) {
                    setCurrentIndex(0);
                } else {
                    setCurrentIndex(setAsCurrent);
                }
                emit currentTrackChanged();
            }

            mAddingTracks = false;
            emit addingTracksChanged();
        });
        watcher->setFuture(future);
    }

    void Queue::removeTrack(int index)
    {
        const std::shared_ptr<QueueTrack> currentTrack(mTracks.at(mCurrentIndex));
        mNotPlayedTracks.removeOne(mTracks.takeAt(index));

        emit trackRemoved(index);

        if (index == mCurrentIndex) {
            if (mCurrentIndex >= mTracks.size()) {
                setCurrentIndex(mTracks.size() - 1);
            } else {
                emit currentIndexChanged();
            }
            emit currentTrackChanged();
        } else {
            setCurrentIndex(mTracks.indexOf(currentTrack));
        }
    }

    void Queue::removeTracks(QVector<int> indexes)
    {
        const std::shared_ptr<QueueTrack> currentTrack(mTracks.at(mCurrentIndex));

        std::reverse(indexes.begin(), indexes.end());
        for (int index : indexes) {
            mNotPlayedTracks.removeOne(mTracks.takeAt(index));
        }

        emit tracksRemoved(indexes);

        if (indexes.contains(mCurrentIndex)) {
            if (mCurrentIndex >= mTracks.size()) {
                setCurrentIndex(mTracks.size() - 1);
            }
            emit currentTrackChanged();
        } else {
            setCurrentIndex(mTracks.indexOf(currentTrack));
        }
    }

    void Queue::clear()
    {
        mTracks.clear();
        mNotPlayedTracks.clear();
        emit cleared();
        setCurrentIndex(-1);
        emit currentTrackChanged();
    }

    void Queue::next()
    {
        if (mShuffle) {
            if (mNotPlayedTracks.length() == 1) {
                resetNotPlayedTracks();
            }
            mNotPlayedTracks.removeOne(mTracks.at(mCurrentIndex));
            setCurrentIndex(mTracks.indexOf(mNotPlayedTracks.at(qrand() % mNotPlayedTracks.size())));
        } else {
            if (mCurrentIndex == (mTracks.size() - 1)) {
                setCurrentIndex(0);
            } else {
                setCurrentIndex(mCurrentIndex + 1);
            }
        }

        emit currentTrackChanged();
    }

    void Queue::nextOnEos()
    {
        if (mRepeatMode == RepeatOne) {
            emit currentTrackChanged();
            return;
        }

        if (mShuffle) {
            const std::shared_ptr<QueueTrack>& track = mTracks.at(mCurrentIndex);
            mNotPlayedTracks.removeOne(track);
            if (mNotPlayedTracks.size() == 0) {
                if (mRepeatMode == RepeatAll) {
                    resetNotPlayedTracks();
                    mNotPlayedTracks.removeOne(track);
                } else {
                    return;
                }
            }
            setCurrentIndex(mTracks.indexOf(mNotPlayedTracks.at(qrand() % mNotPlayedTracks.size())));
        } else {
            if (mCurrentIndex == (mTracks.size() - 1)) {
                if (mRepeatMode == RepeatAll) {
                    setCurrentIndex(0);
                } else {
                    return;
                }
            } else {
                setCurrentIndex(mCurrentIndex + 1);
            }
        }

        emit currentTrackChanged();
    }

    void Queue::previous()
    {
        if (mShuffle) {
            return;
        }

        if (mCurrentIndex == 0) {
            setCurrentIndex(mTracks.size() - 1);
        } else {
            setCurrentIndex(mCurrentIndex - 1);
        }

        emit currentTrackChanged();
    }

    void Queue::setCurrentToFirstIfNeeded()
    {
        if (mCurrentIndex == -1) {
            setCurrentIndex(0);
            emit currentTrackChanged();
        }
    }

    void Queue::resetNotPlayedTracks()
    {
        mNotPlayedTracks = mTracks;
    }

    void Queue::reset()
    {
        clear();
        setCurrentIndex(-1);
        emit currentTrackChanged();
    }
}
