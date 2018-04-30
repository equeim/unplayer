/*
 * Unplayer
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
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

#include "libraryutils.h"
#include "playlistutils.h"
#include "settings.h"
#include "stdutils.h"
#include "tagutils.h"

namespace unplayer
{
    namespace
    {
        const QString dbConnectionName(QLatin1String("unplayer_queue"));
    }

    namespace
    {
        void seedPRNG(void)
        {
            static bool didSeedPRNG = false;
            if (!didSeedPRNG) {
                qsrand(QTime::currentTime().msec());
                didSeedPRNG = true;
            }
        }
    }

    QueueTrack::QueueTrack(const QString& filePath,
                           const QString& title,
                           int duration,
                           const QString& artist,
                           const QString& album,
                           const QString& mediaArt,
                           const QByteArray& mediaArtData)
        : filePath(filePath),
          title(title),
          duration(duration),
          artist(artist),
          album(album),
          mediaArtFilePath(mediaArt)
    {
        mediaArtPixmap.loadFromData(mediaArtData);
    }

    Queue::Queue(QObject* parent)
        : QObject(parent),
          mCurrentIndex(-1),
          mShuffle(false),
          mRepeatMode(NoRepeat),
          mAddingTracks(false)
    {
        seedPRNG();
        QObject::connect(LibraryUtils::instance(), &LibraryUtils::mediaArtChanged, this, [=]() {
            QSqlDatabase::database().transaction();
            for (const std::shared_ptr<QueueTrack>& track : mTracks) {
                QSqlQuery query;
                query.prepare(QStringLiteral("SELECT mediaArt FROM tracks WHERE filePath = ?"));
                query.addBindValue(track->filePath);
                if (query.exec()) {
                    if (query.next()) {
                        track->mediaArtFilePath = query.value(0).toString();
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

    const std::vector<std::shared_ptr<QueueTrack>>& Queue::tracks() const
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
        if (mCurrentIndex >= 0) {
            return mTracks[mCurrentIndex]->filePath;
        }
        return QString();
    }

    QString Queue::currentTitle() const
    {
        if (mCurrentIndex >= 0) {
            return mTracks[mCurrentIndex]->title;
        }
        return QString();
    }

    QString Queue::currentArtist() const
    {
        if (mCurrentIndex >= 0) {
            return mTracks[mCurrentIndex]->artist;
        }
        return QString();
    }

    QString Queue::currentAlbum() const
    {
        if (mCurrentIndex >= 0) {
            return mTracks[mCurrentIndex]->album;
        }
        return QString();
    }

    QString Queue::currentMediaArt() const
    {
        if (mCurrentIndex >= 0) {
            const QueueTrack* track = mTracks[mCurrentIndex].get();
            if (!track->mediaArtFilePath.isEmpty()) {
                return track->mediaArtFilePath;
            }
            if (!track->mediaArtPixmap.isNull()) {
                return QString::fromLatin1("image://%1/%2").arg(QueueImageProvider::providerId, track->filePath);
            }
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

    void Queue::setRepeatMode(int mode)
    {
        mRepeatMode = static_cast<RepeatMode>(mode);
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

        if (clearQueue) {
            clear();
        }

        // FIXME: use proper move capture on C++14
        auto future = QtConcurrent::run(std::bind([this](QStringList& trackPaths) {
            std::vector<std::shared_ptr<QueueTrack>> tracks;

            const QMimeDatabase mimeDb;

            for (int i = 0, max = trackPaths.size(); i < max; ++i) {
                const QString filePath(trackPaths[i]);
                if (contains(PlaylistUtils::playlistsMimeTypes, mimeDb.mimeTypeForFile(filePath, QMimeDatabase::MatchExtension).name())) {
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
                db.setDatabaseName(LibraryUtils::instance()->databaseFilePath());
                if (!db.open()) {
                    qWarning() << "failed to open database" << db.lastError();
                }

                db.transaction();

                std::unordered_map<QString, QString> mediaArtDirectoriesHash;

                const bool useDirectoryMediaArt = Settings::instance()->useDirectoryMediaArt();

                for (const QString& filePath : const_cast<const QStringList&>(trackPaths)) {
                    bool found = false;
                    for (auto& track : std::vector<std::shared_ptr<QueueTrack>>(mTracks)) {
                        if (track->filePath == filePath) {
                            tracks.push_back(std::move(track));
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        continue;
                    }

                    const QFileInfo fileInfo(filePath);
                    if (!fileInfo.exists()) {
                        qWarning() << "file does not exist:" << filePath;
                        continue;
                    }

                    QString title;
                    QStringList artists;
                    QStringList albums;
                    int duration;
                    QString mediaArtFilePath;
                    QByteArray mediaArtData;

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
                                    mediaArtFilePath = query.value(5).toString();

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
                        tagutils::Info info(tagutils::getTrackInfo(fileInfo, mimeDb.mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name()));
                        title = std::move(info.title);
                        artists = std::move(info.artists);
                        albums = std::move(info.albums);
                        duration = info.duration;
                        if (useDirectoryMediaArt) {
                            mediaArtFilePath = LibraryUtils::findMediaArtForDirectory(mediaArtDirectoriesHash, fileInfo.path());
                            if (mediaArtFilePath.isEmpty()) {
                                mediaArtData = std::move(info.mediaArtData);
                            }
                        } else {
                            if (info.mediaArtData.isEmpty()) {
                                mediaArtFilePath = LibraryUtils::findMediaArtForDirectory(mediaArtDirectoriesHash, fileInfo.path());
                            } else {
                                mediaArtData = std::move(info.mediaArtData);
                            }
                        }
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

                    tracks.push_back(std::make_shared<QueueTrack>(std::move(filePath),
                                                                  std::move(title),
                                                                  duration,
                                                                  std::move(artist),
                                                                  std::move(album),
                                                                  std::move(mediaArtFilePath),
                                                                  std::move(mediaArtData)));
                }

                db.commit();
            }
            QSqlDatabase::removeDatabase(dbConnectionName);

            return tracks;
        }, std::move(trackPaths)));

        using FutureWatcher = QFutureWatcher<std::vector<std::shared_ptr<QueueTrack>>>;
        auto watcher = new FutureWatcher(this);
        QObject::connect(watcher, &FutureWatcher::finished, this, [=]() {
            auto tracks(watcher->result());

            emit tracksAboutToBeAdded(tracks.size());
            for (auto& track : tracks) {
                mNotPlayedTracks.push_back(track.get());
                mTracks.push_back(std::move(track));
            }
            emit tracksAdded();

            if (mCurrentIndex == -1 && !mTracks.empty()) {
                if (setAsCurrent == -1) {
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
        const std::shared_ptr<QueueTrack> current(mTracks[mCurrentIndex]);

        emit trackAboutToBeRemoved(index);

        erase_one(mNotPlayedTracks, mTracks[index].get());
        mTracks.erase(mTracks.begin() + index);

        emit trackRemoved();

        if (index == mCurrentIndex) {
            if (mCurrentIndex >= static_cast<int>(mTracks.size())) {
                setCurrentIndex(static_cast<int>(mTracks.size() - 1));
            } else {
                emit currentIndexChanged();
            }
            emit currentTrackChanged();
        } else {
            setCurrentIndex(index_of(mTracks, current));
        }
    }

    void Queue::removeTracks(std::vector<int> indexes)
    {
        const std::shared_ptr<QueueTrack> current(mTracks[mCurrentIndex]);

        std::reverse(indexes.begin(), indexes.end());
        for (int index : indexes) {
            emit trackAboutToBeRemoved(index);
            erase_one(mNotPlayedTracks, mTracks[index].get());
            mTracks.erase(mTracks.begin() + index);
            emit trackRemoved();
        }

        if (contains(indexes, mCurrentIndex)) {
            if (mCurrentIndex >= static_cast<int>(mTracks.size())) {
                setCurrentIndex(mTracks.size() - 1);
            }
            emit currentTrackChanged();
        } else {
            setCurrentIndex(index_of(mTracks, current));
        }
    }

    void Queue::clear()
    {
        emit aboutToBeCleared();
        mTracks.clear();
        mNotPlayedTracks.clear();
        emit cleared();
        setCurrentIndex(-1);
        emit currentTrackChanged();
    }

    void Queue::next()
    {
        if (mShuffle) {
            if (mNotPlayedTracks.size() == 1) {
                resetNotPlayedTracks();
            }
            erase_one(mNotPlayedTracks, mTracks[mCurrentIndex].get());
            mTracks.erase(mTracks.begin() + mCurrentIndex);
            setCurrentIndex(index_of(mTracks, mNotPlayedTracks[qrand() % mNotPlayedTracks.size()]));
        } else {
            if (mCurrentIndex == static_cast<int>(mTracks.size() - 1)) {
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
            const std::shared_ptr<QueueTrack>& track = mTracks[mCurrentIndex];
            erase_one(mNotPlayedTracks, track.get());
            if (mNotPlayedTracks.size() == 0) {
                if (mRepeatMode == RepeatAll) {
                    resetNotPlayedTracks();
                    erase_one(mNotPlayedTracks, track.get());
                } else {
                    return;
                }
            }
            setCurrentIndex(index_of(mTracks, mNotPlayedTracks[qrand() % mNotPlayedTracks.size()]));
        } else {
            if (mCurrentIndex == static_cast<int>(mTracks.size() - 1)) {
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
        mNotPlayedTracks.clear();
        mNotPlayedTracks.reserve(mTracks.size());
        for (const auto& track : mTracks) {
            mNotPlayedTracks.push_back(track.get());
        }
    }

    void Queue::reset()
    {
        clear();
        setCurrentIndex(-1);
        emit currentTrackChanged();
    }

    const QString QueueImageProvider::providerId(QLatin1String("queue"));

    QueueImageProvider::QueueImageProvider(const Queue* queue)
        : QQuickImageProvider(QQuickImageProvider::Pixmap),
          mQueue(queue)
    {

    }

    QPixmap QueueImageProvider::requestPixmap(const QString& id, QSize*, const QSize& requestedSize)
    {
        for (const auto& track : mQueue->tracks()) {
            if (track->filePath == id) {
                const QPixmap& pixmap = track->mediaArtPixmap;
                if (requestedSize.isValid()) {
                    QSize newSize(requestedSize);
                    if (newSize.width() == 0) {
                        newSize.setWidth(pixmap.width());
                    }
                    if (newSize.height() == 0) {
                        newSize.setHeight(pixmap.height());
                    }
                    return pixmap.scaled(newSize, Qt::KeepAspectRatio);
                }
                return pixmap;
            }
        }
        return QPixmap();
    }
}
