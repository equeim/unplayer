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
#include <functional>
#include <unordered_map>

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QMimeDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QUrl>
#include <QUuid>
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

        void seedPRNG()
        {
            static bool didSeedPRNG = false;
            if (!didSeedPRNG) {
                qsrand(QTime::currentTime().msec());
                didSeedPRNG = true;
            }
        }

        QString createTrackId()
        {
            QString id(QLatin1String("/"));
            id += QUuid::createUuid().toString().remove(0, 1).remove(QLatin1Char('-'));
            id.chop(1);
            return id;
        }

        long long toMsecsSinceEpoch(const QDateTime& date)
        {
            if (date.isValid()) {
                return date.toMSecsSinceEpoch();
            }
            return -1;
        }
    }

    QueueTrack::QueueTrack(const QString& trackId,
                           const QUrl& url,
                           const QString& title,
                           int duration,
                           const QString& artist,
                           const QString& album,
                           const QString& mediaArtFilePath,
                           const QByteArray& mediaArtData,
                           long long modificationTime)
        : trackId(trackId),
          url(url),
          title(title),
          duration(duration),
          artist(artist),
          album(album),
          mediaArtFilePath(mediaArtFilePath),
          modificationTime(modificationTime)
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
                if (track->url.isLocalFile()) {
                    const QString filePath(track->url.path());
                    QSqlQuery query;
                    query.prepare(QStringLiteral("SELECT mediaArt FROM tracks WHERE filePath = ?"));
                    query.addBindValue(filePath);
                    if (query.exec()) {
                        if (query.next()) {
                            track->mediaArtFilePath = query.value(0).toString();
                        }
                    } else {
                        qWarning() << "failed to get media art from database for track:" << filePath;
                    }
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

    QUrl Queue::currentUrl() const
    {
        if (mCurrentIndex >= 0) {
            return mTracks[mCurrentIndex]->url;
        }
        return QUrl();
    }

    bool Queue::isCurrentLocalFile() const
    {
        if (mCurrentIndex >= 0) {
            return mTracks[mCurrentIndex]->url.isLocalFile();
        }
        return false;
    }

    QString Queue::currentFilePath() const
    {
        if (mCurrentIndex >= 0) {
            const QUrl& url = mTracks[mCurrentIndex]->url;
            if (url.isLocalFile()) {
                return url.path();
            }
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
            if (track->url.isLocalFile()) {
                if (!track->mediaArtFilePath.isEmpty()) {
                    return track->mediaArtFilePath;
                }
                if (!track->mediaArtPixmap.isNull()) {
                    return QString::fromLatin1("image://%1/%2").arg(QueueImageProvider::providerId, track->url.toString());
                }
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

    void Queue::addTracksFromUrls(const QStringList& trackUrls, bool clearQueue, int setAsCurrent)
    {
        if (mAddingTracks) {
            return;
        }

        if (trackUrls.empty()) {
            return;
        }

        mAddingTracks = true;
        emit addingTracksChanged();

        std::vector<std::shared_ptr<QueueTrack>> oldTracks;
        oldTracks.reserve(mTracks.size());

        if (clearQueue) {
            emit aboutToBeCleared();
            for (auto& track : mTracks) {
                if (track->modificationTime > 0) {
                    oldTracks.push_back(std::move(track));
                }
            }
            clear(false);
        } else {
            for (const auto& track : mTracks) {
                if (track->modificationTime > 0) {
                    oldTracks.push_back(track);
                }
            }
        }

        const QUrl setAsCurrentUrl([&]() {
            if (setAsCurrent < 0 || setAsCurrent >= trackUrls.size()) {
                return QUrl();
            }
            const QString urlString(trackUrls[setAsCurrent]);
            QUrl url(urlString);
            if (url.isRelative()) {
                url = QUrl::fromLocalFile(urlString);
            }
            return url;
        }());

        // FIXME: use capture initializers on C++14
        auto future = QtConcurrent::run(std::bind([](QStringList& trackUrls, std::vector<std::shared_ptr<QueueTrack>>& oldTracks) {
            QTime time;
            time.start();

            std::vector<std::shared_ptr<QueueTrack>> newTracks;

            std::vector<QUrl> existingTracks;
            existingTracks.reserve(trackUrls.size());
            std::vector<QString> tracksToQuery;
            tracksToQuery.reserve(trackUrls.size());

            std::unordered_map<QUrl, std::shared_ptr<QueueTrack>> tracksMap;

            struct TrackHandler
            {
                std::vector<std::shared_ptr<QueueTrack>>& oldTracks;
                const std::vector<std::shared_ptr<QueueTrack>>::iterator oldTracksBegin;
                std::vector<std::shared_ptr<QueueTrack>>::iterator oldTracksEnd;

                std::unordered_map<QUrl, std::shared_ptr<QueueTrack>>& tracksMap;
                std::unordered_map<QUrl, std::shared_ptr<QueueTrack>>::iterator tracksMapEnd;

                std::vector<QUrl>& existingTracks;
                std::vector<QString>& tracksToQuery;

                std::unordered_set<QString> playlists;

                void processTrack(const QUrl& url)
                {
                    if (url.isLocalFile()) {
                        const QFileInfo fileInfo(url.path());
                        if (fileInfo.isFile() && fileInfo.isReadable()) {
                            if (contains(PlaylistUtils::playlistsExtensions, fileInfo.suffix()) &&
                                    !contains(playlists, fileInfo.absoluteFilePath())) {
                                playlists.insert(fileInfo.absoluteFilePath());
                                std::vector<PlaylistTrack> playlistTracks(PlaylistUtils::parsePlaylist(url.path()));
                                existingTracks.reserve(existingTracks.size() + playlistTracks.size());
                                tracksToQuery.reserve(tracksToQuery.size() + playlistTracks.size());
                                for (const PlaylistTrack& playlistTrack : playlistTracks) {
                                    if (playlistTrack.url.isLocalFile()) {
                                        processTrack(playlistTrack.url);
                                    } else {
                                        existingTracks.push_back(playlistTrack.url);
                                        tracksMap.insert({playlistTrack.url, std::make_shared<QueueTrack>(createTrackId(),
                                                                                                          playlistTrack.url,
                                                                                                          playlistTrack.title,
                                                                                                          playlistTrack.duration,
                                                                                                          playlistTrack.artist,
                                                                                                          QString(),
                                                                                                          QString(),
                                                                                                          QByteArray(),
                                                                                                          -1)});
                                    }
                                }
                            } else {
                                if (tracksMap.find(url) == tracksMapEnd) {
                                    const auto modificationTime = toMsecsSinceEpoch(fileInfo.lastModified());
                                    auto found(std::find_if(oldTracksBegin, oldTracksEnd, [&url, &modificationTime](const std::shared_ptr<QueueTrack>& track) {
                                        return track->url == url && track->modificationTime == modificationTime;
                                    }));
                                    if (found == oldTracks.end()) {
                                        tracksToQuery.push_back(url.path());
                                    } else {
                                        tracksMap.insert({url, std::move(*found)});
                                        tracksMapEnd = tracksMap.end();
                                        oldTracks.erase(found);
                                        oldTracksEnd = oldTracks.end();
                                    }
                                    existingTracks.push_back(std::move(url));
                                }
                            }
                        } else {
                            qWarning() << "file" << fileInfo.filePath() << "is not readable";
                        }
                    } else {
                        existingTracks.push_back(std::move(url));
                    }
                }
            };

            {
                TrackHandler handler{oldTracks,
                                     oldTracks.begin(),
                                     oldTracks.end(),
                                     tracksMap,
                                     tracksMap.end(),
                                     existingTracks,
                                     tracksToQuery};
                for (QString& urlString : trackUrls) {
                    const QUrl url([&urlString]() {
                        if (urlString.startsWith(QLatin1Char('/'))) {
                            return QUrl::fromLocalFile(urlString);
                        }
                        return QUrl(urlString);
                    }());
                    if (!url.isRelative()) {
                        handler.processTrack(url);
                    }
                }
            }

            const auto makeTrack = [](const QUrl& url,
                                      QString&& title,
                                      int duration,
                                      QStringList&& artists,
                                      QStringList&& albums,
                                      QString&& mediaArtFilePath,
                                      QByteArray&& mediaArtData,
                                      long long modificationTime) {
                QString artist;
                artists.removeDuplicates();
                if (artists.isEmpty()) {
                    artist = qApp->translate("unplayer", "Unknown artist");
                } else {
                    artist = artists.join(QLatin1String(", "));
                }

                QString album;
                albums.removeDuplicates();
                if (albums.isEmpty()) {
                    album = qApp->translate("unplayer", "Unknown artist");
                } else {
                    album = albums.join(QLatin1String(", "));
                }

                return std::make_shared<QueueTrack>(createTrackId(),
                                                    url,
                                                    std::move(title),
                                                    duration,
                                                    std::move(artist),
                                                    std::move(album),
                                                    std::move(mediaArtFilePath),
                                                    std::move(mediaArtData),
                                                    modificationTime);
            };

            {
                auto db = QSqlDatabase::addDatabase(LibraryUtils::databaseType, dbConnectionName);
                db.setDatabaseName(LibraryUtils::instance()->databaseFilePath());
                if (!db.open()) {
                    qWarning() << "failed to open database" << db.lastError();
                }

                db.transaction();

                const int maxParametersCount = 999;
                for (int i = 0, max = tracksToQuery.size(); i < max; i += maxParametersCount) {
                    const int count = [&]() -> int {
                        const int left = max - i;
                        if (left > maxParametersCount) {
                            return maxParametersCount;
                        }
                        return left;
                    }();
                    QString queryString(QLatin1String("SELECT filePath, modificationTime, title, artist, album, duration, mediaArt FROM tracks WHERE filePath IN (?"));
                    queryString.reserve(queryString.size() + (count - 1) * 2 + 1);
                    for (int j = 1; j < count; ++j) {
                        queryString.push_back(QStringLiteral(",?"));
                    }
                    queryString.push_back(QLatin1Char(')'));

                    QSqlQuery query(db);
                    query.prepare(queryString);
                    for (int j = i, max = i + count; j < max; ++j) {
                        query.addBindValue(tracksToQuery[j]);
                    }


                    if (query.exec()) {
                        QString previousFilePath;
                        QStringList artists;
                        QStringList albums;
                        bool shouldInsert = false;

                        const auto insert = [&]() {
                            const QUrl url(QUrl::fromLocalFile(previousFilePath));
                            tracksMap.insert({url, makeTrack(url,
                                                             query.value(2).toString(),
                                                             query.value(4).toInt(),
                                                             std::move(artists),
                                                             std::move(albums),
                                                             query.value(6).toString(),
                                                             QByteArray(),
                                                             query.value(1).toLongLong())});
                        };

                        while (query.next()) {
                            QString filePath(query.value(0).toString());

                            if (filePath != previousFilePath) {
                                // insert previous
                                if (!previousFilePath.isEmpty() && shouldInsert) {
                                    insert();
                                }

                                const QFileInfo info(filePath);
                                shouldInsert = info.isFile() && info.isReadable() && (query.value(1).toLongLong() == toMsecsSinceEpoch(info.lastModified()));
                            }

                            if (shouldInsert) {
                                artists.push_back(query.value(3).toString());
                                albums.push_back(query.value(4).toString());
                            }

                            previousFilePath = std::move(filePath);
                        }

                        if (shouldInsert && query.previous()) {
                            // insert last
                            insert();
                        }
                    } else {
                        qWarning() << query.lastError();
                    }
                }

                db.commit();
            }
            QSqlDatabase::removeDatabase(dbConnectionName);

            const QMimeDatabase mimeDb;

            std::unordered_map<QString, QString> mediaArtDirectoriesHash;
            const bool preferDirectoryMediaArt = Settings::instance()->useDirectoryMediaArt();

            newTracks.reserve(existingTracks.size());
            const auto tracksMapEnd(tracksMap.end());
            for (QUrl& url : existingTracks) {
                const auto found = tracksMap.find(url);
                if (found == tracksMapEnd) {
                    if (url.isLocalFile()) {
                        const QString filePath(url.path());
                        const QFileInfo fileInfo(filePath);
                        tagutils::Info info(tagutils::getTrackInfo(filePath, mimeDb.mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name()));
                        QString mediaArtFilePath;
                        QByteArray mediaArtData;
                        if (preferDirectoryMediaArt) {
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
                        newTracks.push_back(makeTrack(std::move(url),
                                                      std::move(info.title),
                                                      info.duration,
                                                      std::move(info.artists),
                                                      std::move(info.albums),
                                                      std::move(mediaArtFilePath),
                                                      std::move(mediaArtData),
                                                      toMsecsSinceEpoch(fileInfo.lastModified())));
                    } else {
                        newTracks.push_back(std::make_shared<QueueTrack>(createTrackId(),
                                                                         url,
                                                                         url.toString(),
                                                                         -1,
                                                                         QString(),
                                                                         QString(),
                                                                         QString(),
                                                                         QByteArray(),
                                                                         -1));
                    }
                } else {
                    newTracks.push_back(std::make_shared<QueueTrack>(*(found->second.get())));
                }

                QueueTrack* track = newTracks.back().get();
                if (track->title.isEmpty()) {
                    if (track->url.isLocalFile()) {
                        track->title = QFileInfo(track->url.path()).fileName();
                    } else {
                        track->title = track->url.toString();
                    }
                }
            }

            return newTracks;
        }, trackUrls, std::move(oldTracks)));

        using FutureWatcher = QFutureWatcher<std::vector<std::shared_ptr<QueueTrack>>>;
        auto watcher = new FutureWatcher(this);
        QObject::connect(watcher, &FutureWatcher::finished, this, [=]() {
            addingTracksCallback(watcher->result(), setAsCurrent, setAsCurrentUrl);
        });
        watcher->setFuture(future);
    }

    void Queue::addTrackFromUrl(const QString& trackUrl)
    {
        addTracksFromUrls({trackUrl});
    }

    void Queue::addTracksFromLibrary(const std::vector<LibraryTrack>& libraryTracks, bool clearQueue, int setAsCurrent)
    {
        if (mAddingTracks) {
            return;
        }

        if (libraryTracks.empty()) {
            return;
        }

        mAddingTracks = true;
        emit addingTracksChanged();

        if (clearQueue) {
            clear();
        }

        const QUrl setAsCurrentUrl([&]() {
            if (setAsCurrent < 0 || setAsCurrent >= libraryTracks.size()) {
                return QUrl();
            }
            return QUrl::fromLocalFile(libraryTracks[setAsCurrent].filePath);
        }());

        // FIXME: use capture initializers on C++14
        auto future = QtConcurrent::run(std::bind([](std::vector<LibraryTrack>& libraryTracks) {
            std::vector<std::shared_ptr<QueueTrack>> newTracks;
            newTracks.reserve(libraryTracks.size());

            for (auto& libraryTrack : libraryTracks) {
                QString& filePath = libraryTrack.filePath;

                const QFileInfo info(filePath);

                if (!info.isFile()) {
                    qWarning() << "file" << filePath << "does not exist or not a file";
                    continue;
                }

                if (!info.isReadable()) {
                    qWarning() << "file" << filePath << "is not readable";
                    continue;
                }

                newTracks.push_back(std::make_shared<QueueTrack>(createTrackId(),
                                                                 QUrl::fromLocalFile(libraryTrack.filePath),
                                                                 std::move(libraryTrack.title),
                                                                 libraryTrack.duration,
                                                                 std::move(libraryTrack.artist),
                                                                 std::move(libraryTrack.album),
                                                                 std::move(libraryTrack.mediaArtPath),
                                                                 QByteArray(),
                                                                 -1));
            }

            return newTracks;
        }, libraryTracks));

        using FutureWatcher = QFutureWatcher<std::vector<std::shared_ptr<QueueTrack>>>;
        auto watcher = new FutureWatcher(this);
        QObject::connect(watcher, &FutureWatcher::finished, this, [=]() {
            addingTracksCallback(watcher->result(), setAsCurrent, setAsCurrentUrl);
        });
        watcher->setFuture(future);
    }

    void Queue::addTrackFromLibrary(const LibraryTrack& libraryTrack, bool clearQueue, int setAsCurrent)
    {
        addTracksFromLibrary({libraryTrack}, clearQueue, setAsCurrent);
    }

    LibraryTrack Queue::getTrack(int index)
    {
        const QueueTrack* track = mTracks[index].get();
        return {track->url.toString(),
                track->title,
                track->artist,
                track->album,
                track->duration,
                track->mediaArtFilePath};
    }

    std::vector<LibraryTrack> Queue::getTracks(const std::vector<int>& indexes)
    {
        std::vector<LibraryTrack> tracks;
        tracks.reserve(indexes.size());
        for (int index : indexes) {
            tracks.push_back(getTrack(index));
        }
        return tracks;
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

        std::sort(indexes.begin(), indexes.end(), std::greater<int>());
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

    void Queue::clear(bool emitAbout)
    {
        if (emitAbout) {
            emit aboutToBeCleared();
        }
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
            if (mNotPlayedTracks.empty()) {
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

    void Queue::addingTracksCallback(std::vector<std::shared_ptr<QueueTrack>>&& tracks, int setAsCurrent, const QUrl& setAsCurrentUrl)
    {
        emit tracksAboutToBeAdded(tracks.size());

        setAsCurrent += mTracks.size();

        mNotPlayedTracks.reserve(mNotPlayedTracks.size() + tracks.size());
        mTracks.reserve(mTracks.size() + tracks.size());
        for (auto& track : tracks) {
            mNotPlayedTracks.push_back(track.get());
            mTracks.push_back(std::move(track));
        }
        emit tracksAdded();

        if (mCurrentIndex == -1 && !mTracks.empty()) {
            if (setAsCurrent < 0 || setAsCurrent >= mTracks.size()) {
                setCurrentIndex(0);
            } else {
                if (mTracks[setAsCurrent]->url == setAsCurrentUrl) {
                    setCurrentIndex(setAsCurrent);
                } else {
                    const auto found(std::find_if(mTracks.begin(), mTracks.end(), [&setAsCurrentUrl](const std::shared_ptr<QueueTrack>& track) {
                        return track->url == setAsCurrentUrl;
                    }));
                    if (found == mTracks.end()) {
                        setCurrentIndex(0);
                    } else {
                        setCurrentIndex(found - mTracks.begin());
                    }
                }
            }
            emit currentTrackChanged();
        }

        mAddingTracks = false;
        emit addingTracksChanged();
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
            if (track->url.toString() == id) {
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
