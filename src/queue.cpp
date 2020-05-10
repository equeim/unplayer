/*
 * Unplayer
 * Copyright (C) 2015-2019 Alexey Rochev <equeim@gmail.com>
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
#include <random>
#include <unordered_map>

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QUrl>
#include <QUuid>
#include <QtConcurrentRun>

#include "libraryutils.h"
#include "mediaartutils.h"
#include "playlistutils.h"
#include "settings.h"
#include "sqlutils.h"
#include "tagutils.h"
#include "utilsfunctions.h"

namespace unplayer
{
    namespace
    {
        const QLatin1String dbConnectionName("unplayer_queue");

        size_t randomIndex(size_t count)
        {
            static std::default_random_engine random([]() {
                std::seed_seq seed{std::random_device{}(), static_cast<std::seed_seq::result_type>(QDateTime::currentMSecsSinceEpoch())};
                return std::default_random_engine(seed);
            }());
            std::uniform_int_distribution<> dist(0, static_cast<int>(count) - 1);
            return static_cast<size_t>(dist(random));
        }

        inline QString joinStrings(const QStringList& strings)
        {
            return strings.join(QLatin1String(", "));
        }

        inline QUrl urlFromString(const QString& string)
        {
            QUrl url(string);
            if (url.isRelative()) {
                url.setScheme(QLatin1String("file"));
            }
            return url;
        }

        class TracksAdder
        {
        public:
            static std::vector<std::shared_ptr<QueueTrack>> addTracksFromUrls(const QStringList& trackUrls, std::vector<std::shared_ptr<QueueTrack>>&& oldTracks)
            {
                std::vector<std::shared_ptr<QueueTrack>> newTracks;

                qInfo("Start adding tracks from urls, count = %d, old tracks count = %zu", trackUrls.size(), oldTracks.size());
                QElapsedTimer timer;
                timer.start();

                auto existenceResult(checkTracksExistence(trackUrls, std::move(oldTracks)));
                auto& tracksMap = existenceResult.tracksMap;

                if (!queryTracks(std::move(existenceResult.tracksToQuery), tracksMap)) {
                    return newTracks;
                }

                auto& existingTracks = existenceResult.existingTracks;
                newTracks.reserve(existingTracks.size());
                QMimeDatabase mimeDb;

                for (QUrl& url : existingTracks) {
                    if (!qApp) {
                        break;
                    }

                    const auto found(tracksMap.find(url));
                    if (found == tracksMap.end()) {
                        if (url.isLocalFile()) {
                            const QString filePath(url.path());
                            const QFileInfo fileInfo(filePath);
                            tagutils::Info info(tagutils::getTrackInfo(filePath, fileutils::extensionFromSuffix(fileInfo.suffix()), mimeDb));
                            if (info.title.isEmpty()) {
                                info.title = fileInfo.fileName();
                            }
                            info.artists.append(info.albumArtists);
                            info.artists.removeDuplicates();
                            newTracks.push_back(std::make_shared<QueueTrack>(url,
                                                                             info.title,
                                                                             info.duration,
                                                                             joinStrings(info.artists),
                                                                             joinStrings(info.albums),
                                                                             false));
                            tracksMap.emplace(std::move(url), newTracks.back());
                        } else {
                            newTracks.push_back(std::make_shared<QueueTrack>(url,
                                                                             url.toString(),
                                                                             -1,
                                                                             QString(),
                                                                             QString(),
                                                                             false));
                        }
                    } else {
                        const auto& track = found->second;
                        if (track.use_count() == 1) {
                            newTracks.push_back(track);
                        } else {
                            newTracks.push_back(std::make_shared<QueueTrack>(*track));
                        }
                    }
                }

                qInfo("Tracks adding time %lldms", static_cast<long long>(timer.elapsed()));

                return newTracks;
            }

        private:
            struct ExistenceResult
            {
                std::vector<QUrl> existingTracks;
                std::vector<QString> tracksToQuery;
                std::unordered_map<QUrl, std::shared_ptr<QueueTrack>> tracksMap;
            };

            static ExistenceResult checkTracksExistence(const QStringList& trackUrls, std::vector<std::shared_ptr<QueueTrack>> oldTracks)
            {
                std::vector<QUrl> existingTracks;
                existingTracks.reserve(static_cast<size_t>(trackUrls.size()));
                std::vector<QString> tracksToQuery;
                tracksToQuery.reserve(static_cast<size_t>(trackUrls.size()));
                std::unordered_map<QUrl, std::shared_ptr<QueueTrack>> tracksMap;
                tracksMap.reserve(static_cast<size_t>(trackUrls.size()));

                std::unordered_set<QString> playlists;

                std::function<void(const QUrl&)> processTrack;
                processTrack = [&](const QUrl& url) {
                    if (url.isLocalFile()) {
                        const QFileInfo fileInfo(url.path());
                        if (fileInfo.isFile() && fileInfo.isReadable()) {
                            if (PlaylistUtils::isPlaylistExtension(fileInfo.suffix())) {
                                QString absoluteFilePath(fileInfo.absoluteFilePath());
                                if (!contains(playlists, absoluteFilePath)) {
                                    playlists.insert(std::move(absoluteFilePath));
                                    std::vector<PlaylistTrack> playlistTracks(PlaylistUtils::parsePlaylist(url.path()));
                                    existingTracks.reserve(existingTracks.size() + playlistTracks.size());
                                    tracksToQuery.reserve(tracksToQuery.size() + playlistTracks.size());
                                    tracksMap.reserve(tracksMap.size() + playlistTracks.size());
                                    for (const PlaylistTrack& playlistTrack : playlistTracks) {
                                        if (playlistTrack.url.isLocalFile()) {
                                            processTrack(playlistTrack.url);
                                        } else {
                                            existingTracks.push_back(playlistTrack.url);
                                            if (tracksMap.find(playlistTrack.url) == tracksMap.end()) {
                                                tracksMap.insert({playlistTrack.url, std::make_shared<QueueTrack>(playlistTrack.url,
                                                                                                                  playlistTrack.title,
                                                                                                                  playlistTrack.duration,
                                                                                                                  playlistTrack.artist,
                                                                                                                  QString(),
                                                                                                                  false)});
                                            }
                                        }
                                    }
                                }
                            } else {
                                if (tracksMap.find(url) == tracksMap.end()) {
                                    auto found(std::find_if(oldTracks.begin(), oldTracks.end(), [&url](const std::shared_ptr<QueueTrack>& track) {
                                        return track->url == url;
                                    }));
                                    if (found == oldTracks.end()) {
                                        tracksToQuery.push_back(url.path());
                                    } else {
                                        tracksMap.insert({url, std::move(*found)});
                                        oldTracks.erase(found);
                                    }
                                    existingTracks.push_back(url);
                                }
                            }
                        } else {
                            qWarning() << "File is not readable:" << fileInfo.filePath();
                        }
                    } else {
                        existingTracks.push_back(url);
                    }
                };

                for (const QString& urlString : trackUrls) {
                    if (!qApp) {
                        break;
                    }
                    const QUrl url(urlFromString(urlString));
                    if (!url.isRelative()) {
                        processTrack(url);
                    }
                }

                return {std::move(existingTracks),
                        std::move(tracksToQuery),
                        std::move(tracksMap)};
            }

            static bool queryTracks(std::vector<QString> tracksToQuery, std::unordered_map<QUrl, std::shared_ptr<QueueTrack>>& tracksMap)
            {
                const DatabaseConnectionGuard databaseGuard{dbConnectionName};
                QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
                if (!db.isOpen()) {
                    return false;
                }

                std::sort(tracksToQuery.begin(), tracksToQuery.end());
                tracksToQuery.erase(std::unique(tracksToQuery.begin(), tracksToQuery.end()), tracksToQuery.end());

                bool abort = false;

                QSqlQuery query(db);
                size_t previousCount = 0;

                batchedCount(tracksToQuery.size(), LibraryUtils::maxDbVariableCount, [&](size_t first, size_t count) {
                    enum {
                        FilePathField,
                        TitleField,
                        DurationField,
                        ArtistField,
                        AlbumField
                    };

                    if (!qApp) {
                        abort = true;
                    }

                    if (abort) {
                        return;
                    }

                    if (count != previousCount) {
                        QString queryString(QLatin1String("SELECT filePath, tracks.title, duration, artists.title, albums.title "
                                                          "FROM tracks "
                                                          "LEFT JOIN tracks_artists ON tracks_artists.trackId = tracks.id "
                                                          "LEFT JOIN artists ON artists.id = tracks_artists.artistId "
                                                          "LEFT JOIN tracks_albums ON tracks_albums.trackId = tracks.id "
                                                          "LEFT JOIN albums ON albums.id = tracks_albums.albumId "
                                                          "WHERE filePath IN (?"));

                        const auto add(QStringLiteral(",?"));
                        const int addSize = add.size();
                        queryString.reserve(queryString.size() + static_cast<int>(count - 1) * addSize + 1);
                        for (size_t i = 1; i < count; ++i) {
                            queryString += add;
                        }
                        queryString += QLatin1Char(')');

                        if (!query.prepare(queryString)) {
                            qWarning() << query.lastError();
                            abort = true;
                            return;
                        }

                        previousCount = count;
                    }

                    for (size_t i = first, max = first + count; i < max; ++i) {
                        query.addBindValue(tracksToQuery[i]);
                    }

                    if (!query.exec()) {
                        qWarning() << query.lastError();
                        abort = true;
                        return;
                    }

                    QString filePath;
                    QStringList artists;
                    QStringList albums;

                    const auto insert = [&] {
                        const QUrl url(QUrl::fromLocalFile(filePath));
                        QString title(query.value(TitleField).toString());
                        if (title.isEmpty()) {
                            title = QFileInfo(filePath).fileName();
                        }
                        tracksMap.emplace(url,
                                          std::make_shared<QueueTrack>(url,
                                                                       title,
                                                                       query.value(DurationField).toInt(),
                                                                       joinStrings(artists),
                                                                       joinStrings(albums),
                                                                       false));
                        artists.clear();
                        albums.clear();
                    };

                    while (query.next()) {
                        QString newFilePath(query.value(FilePathField).toString());
                        if (newFilePath != filePath) {
                            if (!filePath.isEmpty()) {
                                insert();
                            }
                            filePath = std::move(newFilePath);
                        }
                        const QString artist(query.value(ArtistField).toString());
                        if (!artist.isEmpty() && !artists.contains(artist)) {
                            artists.push_back(artist);
                        }
                        const QString album(query.value(AlbumField).toString());
                        if (!album.isEmpty() && !albums.contains(album)) {
                            albums.push_back(album);
                        }
                    }
                    if (!filePath.isEmpty()) {
                        query.previous();
                        insert();
                    }

                    query.finish();
                });

                return !abort;
            }
        };
    }

    QueueTrack::QueueTrack(const QUrl& url,
                           const QString& title,
                           int duration,
                           const QString& artist,
                           const QString& album,
                           bool filteredSingleAlbum)
        : url(url),
          title(title),
          duration(duration),
          artist(artist),
          album(album),
          filteredSingleAlbum(filteredSingleAlbum)
    {
    }

    void QueueTrack::initTrackId() const
    {
        mTrackId = QLatin1Char('/');
        mTrackId += QUuid::createUuid().toString().remove(0, 1).remove(QLatin1Char('-'));
        mTrackId.chop(1);
    }

    Queue::Queue(QObject* parent)
        : QObject(parent),
          mCurrentIndex(-1),
          mShuffle(false),
          mRepeatMode(NoRepeat),
          mAddingTracks(false)
    {
        QObject::connect(this, &Queue::currentTrackChanged, this, [this] {
            if (!mTracks.empty()) {
                const auto& track = mTracks[static_cast<size_t>(mCurrentIndex)];
                if (track->url.isLocalFile()) {
                    MediaArtUtils::instance()->getMediaArtForFile(track->url.path(),
                                                                  track->filteredSingleAlbum ? track->album : QString(),
                                                                  !track->mediaArtFilePath.isEmpty());
                    return;
                }
            }
            if (!mCurrentMediaArtData.isEmpty()) {
                mCurrentMediaArtData.clear();
                emit mediaArtDataChanged(mCurrentMediaArtData);
            }
            emit mediaArtChanged();
        });

        QObject::connect(MediaArtUtils::instance(), &MediaArtUtils::gotMediaArtForFile, this, [this](const QString& filePath, const QString& mediaArt, const QByteArray& embeddedMediaArtData) {
            if (filePath == currentFilePath()) {
                auto& track = mTracks[static_cast<size_t>(mCurrentIndex)];
                bool changed = false;
                if (track->mediaArtFilePath.isEmpty() && mediaArt != track->mediaArtFilePath) {
                    track->mediaArtFilePath = mediaArt;
                    changed = true;
                }
                if (embeddedMediaArtData != mCurrentMediaArtData) {
                    mCurrentMediaArtData = embeddedMediaArtData;
                    emit mediaArtDataChanged(mCurrentMediaArtData);
                    changed = true;
                }
                if (changed) {
                    emit mediaArtChanged();
                }
            }
        });

        QObject::connect(LibraryUtils::instance(), &LibraryUtils::mediaArtChanged, this, [=]() {
            if (!mTracks.empty()) {
                for (auto& track : mTracks) {
                    track->mediaArtFilePath.clear();
                }
                const auto& track = mTracks[static_cast<size_t>(mCurrentIndex)];
                if (track->url.isLocalFile()) {
                    MediaArtUtils::instance()->getMediaArtForFile(track->url.path(),
                                                                  track->filteredSingleAlbum ? track->album : QString(),
                                                                  false);
                }
            }
        });
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
            return mTracks[static_cast<size_t>(mCurrentIndex)]->url;
        }
        return QUrl();
    }

    bool Queue::isCurrentLocalFile() const
    {
        if (mCurrentIndex >= 0) {
            return mTracks[static_cast<size_t>(mCurrentIndex)]->url.isLocalFile();
        }
        return false;
    }

    QString Queue::currentFilePath() const
    {
        if (mCurrentIndex >= 0) {
            const QUrl& url = mTracks[static_cast<size_t>(mCurrentIndex)]->url;
            if (url.isLocalFile()) {
                return url.path();
            }
        }
        return QString();
    }

    QString Queue::currentTitle() const
    {
        if (mCurrentIndex >= 0) {
            return mTracks[static_cast<size_t>(mCurrentIndex)]->title;
        }
        return QString();
    }

    QString Queue::currentArtist() const
    {
        if (mCurrentIndex >= 0) {
            return mTracks[static_cast<size_t>(mCurrentIndex)]->artist;
        }
        return QString();
    }

    QString Queue::currentAlbum() const
    {
        if (mCurrentIndex >= 0) {
            return mTracks[static_cast<size_t>(mCurrentIndex)]->album;
        }
        return QString();
    }

    QString Queue::currentMediaArt() const
    {
        if (mCurrentIndex >= 0) {
            const QueueTrack* track = mTracks[static_cast<size_t>(mCurrentIndex)].get();
            if (track->url.isLocalFile()) {
                if (!mCurrentMediaArtData.isEmpty() && (!Settings::instance()->useDirectoryMediaArt() || track->mediaArtFilePath.isEmpty())) {
                    return QString::fromLatin1("image://%1/%2").arg(QueueImageProvider::providerId, track->url.toString());
                }
                return track->mediaArtFilePath;
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
        const auto newMode = static_cast<RepeatMode>(mode);
        if (newMode != mRepeatMode) {
            mRepeatMode = newMode;
            emit repeatModeChanged();
        }
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

        if (clearQueue) {
            emit aboutToBeCleared();
        }

        std::vector<std::shared_ptr<QueueTrack>> oldTracks;
        oldTracks.reserve(mTracks.size());

        for (auto& track : mTracks) {
            if (track->url.isLocalFile()) {
                oldTracks.push_back(std::move(track));
            }
        }

        if (clearQueue) {
            clear(false);
        }

        const QUrl setAsCurrentUrl([&]() {
            if (setAsCurrent < 0 || setAsCurrent >= trackUrls.size()) {
                return QUrl();
            }
            return urlFromString(trackUrls[setAsCurrent]);
        }());

        auto future = QtConcurrent::run([trackUrls, oldTracks = std::move(oldTracks)]() mutable {
            return TracksAdder::addTracksFromUrls(trackUrls, std::move(oldTracks));
        });

        onFutureFinished(future, this, [=](std::vector<std::shared_ptr<QueueTrack>>&& tracks) {
            addingTracksCallback(std::move(tracks), setAsCurrent, setAsCurrentUrl);
        });
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
            if (setAsCurrent < 0 || setAsCurrent >= static_cast<int>(libraryTracks.size())) {
                return QUrl();
            }
            return QUrl::fromLocalFile(libraryTracks[static_cast<size_t>(setAsCurrent)].filePath);
        }());

        auto future = QtConcurrent::run([libraryTracks]() {
            std::vector<std::shared_ptr<QueueTrack>> newTracks;
            newTracks.reserve(libraryTracks.size());

            for (const LibraryTrack& libraryTrack : libraryTracks) {
                const QString& filePath = libraryTrack.filePath;
                const QFileInfo info(filePath);
                if (!info.isFile()) {
                    qWarning() << "File does not exist or is not a file:" << filePath;
                    continue;
                }
                if (!info.isReadable()) {
                    qWarning() << "File is not readable:" << filePath;
                    continue;
                }
                newTracks.push_back(std::make_shared<QueueTrack>(QUrl::fromLocalFile(filePath),
                                                                 libraryTrack.title,
                                                                 libraryTrack.duration,
                                                                 libraryTrack.artist,
                                                                 libraryTrack.album,
                                                                 libraryTrack.filteredSingleAlbum));
            }

            return newTracks;
        });

        onFutureFinished(future, this, [=](std::vector<std::shared_ptr<QueueTrack>>&& tracks) {
            addingTracksCallback(std::move(tracks), setAsCurrent, setAsCurrentUrl);
        });
    }

    void Queue::addTrackFromLibrary(const LibraryTrack& libraryTrack, bool clearQueue, int setAsCurrent)
    {
        addTracksFromLibrary({libraryTrack}, clearQueue, setAsCurrent);
    }

    LibraryTrack Queue::getTrack(int index) const
    {
        const QueueTrack* track = mTracks[static_cast<size_t>(index)].get();
        return {0,
                track->url.toString(),
                track->title,
                track->artist,
                track->album,
                track->filteredSingleAlbum,
                track->duration};
    }

    std::vector<LibraryTrack> Queue::getTracks(const std::vector<int>& indexes) const
    {
        std::vector<LibraryTrack> tracks;
        tracks.reserve(indexes.size());
        for (int index : indexes) {
            tracks.push_back(getTrack(index));
        }
        return tracks;
    }

    bool Queue::hasLocalFileForTracks(const std::vector<int>& indexes) const
    {
        for (int index : indexes) {
            if (mTracks[static_cast<decltype(mTracks)::size_type>(index)]->url.isLocalFile()) {
                return true;
            }
        }
        return false;
    }

    QStringList Queue::getTrackPaths(const std::vector<int>& indexes) const
    {
        QStringList tracks;
        tracks.reserve(static_cast<int>(indexes.size()));
        for (int index : indexes) {
            const QueueTrack* track = mTracks[static_cast<decltype(mTracks)::size_type>(index)].get();
            if (track->url.isLocalFile()) {
                tracks.push_back(track->url.path());
            }
        }
        return tracks;
    }

    void Queue::removeTrack(int index)
    {
        const std::shared_ptr<QueueTrack> current(mTracks[static_cast<size_t>(mCurrentIndex)]);

        emit trackAboutToBeRemoved(index);

        erase_one(mNotPlayedTracks, mTracks[static_cast<size_t>(index)].get());
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
            setCurrentIndex(index_of_i(mTracks, current));
        }
    }

    void Queue::removeTracks(std::vector<int> indexes)
    {
        const std::shared_ptr<QueueTrack> current(mTracks[static_cast<size_t>(mCurrentIndex)]);

        std::sort(indexes.begin(), indexes.end(), std::greater<int>());
        for (int index : indexes) {
            emit trackAboutToBeRemoved(index);
            erase_one(mNotPlayedTracks, mTracks[static_cast<size_t>(index)].get());
            mTracks.erase(mTracks.begin() + index);
            emit trackRemoved();
        }

        if (contains(indexes, mCurrentIndex)) {
            if (mCurrentIndex >= static_cast<int>(mTracks.size())) {
                setCurrentIndex(static_cast<int>(mTracks.size()) - 1);
            }
            emit currentTrackChanged();
        } else {
            setCurrentIndex(index_of_i(mTracks, current));
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
            erase_one(mNotPlayedTracks, mTracks[static_cast<size_t>(mCurrentIndex)].get());
            mTracks.erase(mTracks.begin() + mCurrentIndex);
            setCurrentIndex(index_of_i(mTracks, mNotPlayedTracks[randomIndex(mNotPlayedTracks.size())]));
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
            const std::shared_ptr<QueueTrack>& track = mTracks[static_cast<size_t>(mCurrentIndex)];
            erase_one(mNotPlayedTracks, track.get());
            if (mNotPlayedTracks.empty()) {
                if (mRepeatMode == RepeatAll) {
                    resetNotPlayedTracks();
                    erase_one(mNotPlayedTracks, track.get());
                } else {
                    return;
                }
            }
            setCurrentIndex(index_of_i(mTracks, mNotPlayedTracks[randomIndex(mNotPlayedTracks.size())]));
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
            setCurrentIndex(static_cast<int>(mTracks.size()) - 1);
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
        emit tracksAboutToBeAdded(static_cast<int>(tracks.size()));

        // If queue was not cleared
        setAsCurrent += static_cast<int>(mTracks.size());

        mNotPlayedTracks.reserve(mNotPlayedTracks.size() + tracks.size());
        mTracks.reserve(mTracks.size() + tracks.size());
        for (auto& track : tracks) {
            mNotPlayedTracks.push_back(track.get());
            mTracks.push_back(std::move(track));
        }

        emit tracksAdded();

        if (mCurrentIndex == -1 && !mTracks.empty()) {
            bool set = false;
            if (setAsCurrentUrl.isEmpty()) {
                setCurrentIndex(0);
            } else {
                if (setAsCurrent >= 0 &&
                        setAsCurrent < static_cast<int>(mTracks.size()) &&
                        mTracks[static_cast<size_t>(setAsCurrent)]->url == setAsCurrentUrl) {
                    setCurrentIndex(setAsCurrent);
                    set = true;
                } else {
                    const auto found(std::find_if(mTracks.begin(), mTracks.end(), [&setAsCurrentUrl](const std::shared_ptr<QueueTrack>& track) {
                        return track->url == setAsCurrentUrl;
                    }));
                    if (found == mTracks.end()) {
                        setCurrentIndex(0);
                    } else {
                        setCurrentIndex(static_cast<int>(found - mTracks.begin()));
                        set = true;
                    }
                }
            }
            emit currentTrackChanged(set);
        }

        mAddingTracks = false;
        emit addingTracksChanged();
    }

    const QLatin1String QueueImageProvider::providerId("queue");

    QueueImageProvider::QueueImageProvider(const Queue* queue)
        : QQuickImageProvider(QQuickImageProvider::Pixmap)
    {
        QObject::connect(queue, &Queue::mediaArtDataChanged, this, [=](const QByteArray& mediaArtData) {
            std::lock_guard<std::mutex> guard(mMutex);
            mMediaArtData = mediaArtData;
            if (mMediaArtData.isEmpty() && !mPixmap.isNull()) {
                mPixmap = QPixmap();
            }
        });
    }

    QPixmap QueueImageProvider::requestPixmap(const QString&, QSize* size, const QSize& requestedSize)
    {
        QPixmap pixmap;
        {
            QByteArray mediaArtData;
            std::lock_guard<std::mutex> loadingGuard(mLoadingMutex);
            {
                std::lock_guard<std::mutex> guard(mMutex);
                pixmap = mPixmap;
                mediaArtData = mMediaArtData;
            }
            if (!mediaArtData.isEmpty()) {
                pixmap.loadFromData(mediaArtData);
                {
                    std::lock_guard<std::mutex> guard(mMutex);
                    mPixmap = pixmap;
                    mMediaArtData.clear();
                }
            }
        }

        if (!pixmap.isNull()) {
            *size = pixmap.size();
            if (requestedSize.isValid() && !requestedSize.isNull()) {
                return pixmap.scaled(requestedSize.width() > 0 ? requestedSize.width() : std::numeric_limits<int>::max(),
                                     requestedSize.height() > 0 ? requestedSize.height() : std::numeric_limits<int>::max(),
                                     Qt::KeepAspectRatio);
            }
        }
        return pixmap;
    }
}
