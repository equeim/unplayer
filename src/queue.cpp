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
#include <limits>
#include <random>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QUrl>
#include <QUuid>
#include <QtConcurrentRun>

#include "fileutils.h"
#include "libraryutils.h"
#include "mediaartutils.h"
#include "playlistutils.h"
#include "settings.h"
#include "stdutils.h"
#include "tagutils.h"
#include "tracksquery.h"
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
            static std::vector<std::shared_ptr<QueueTrack>> addTracksFromUrls(const QStringList& trackUrls, std::unordered_map<QUrl, std::shared_ptr<QueueTrack>>&& oldTracks)
            {
                qInfo("Start adding tracks from urls, count = %d, old tracks count = %zu", trackUrls.size(), oldTracks.size());
                QElapsedTimer timer;
                timer.start();

                auto createTracksResult(createTracks(trackUrls, std::move(oldTracks)));

                if (!queryTracksByPaths(std::move(createTracksResult.tracksToQuery), createTracksResult.tracksToQueryMap, dbConnectionName)) {
                    return {};
                }

                extractTrackInfos(std::move(createTracksResult.tracksToQueryMap));

                qInfo("Finished adding tracks: %lldms", static_cast<long long>(timer.elapsed()));

                return std::move(createTracksResult.newTracks);
            }

        private:
            struct CreateTracksResult
            {
                std::vector<std::shared_ptr<QueueTrack>> newTracks;
                std::set<QString> tracksToQuery;
                std::unordered_multimap<QString, QueueTrack*> tracksToQueryMap;
            };

            static CreateTracksResult createTracks(const QStringList& trackUrls, std::unordered_map<QUrl, std::shared_ptr<QueueTrack>> oldTracks)
            {
                qInfo("Start creating tracks, count=%d", trackUrls.size());

                QElapsedTimer timer;
                timer.start();

                std::vector<std::shared_ptr<QueueTrack>> newTracks;
                newTracks.reserve(static_cast<size_t>(trackUrls.size()));

                std::set<QString> tracksToQuery;

                std::unordered_multimap<QString, QueueTrack*> tracksToQueryMap;
                tracksToQueryMap.reserve(static_cast<size_t>(trackUrls.size()));

                std::unordered_set<QString> playlists;

                const auto oldTracksEnd(oldTracks.end());

                std::function<void(const QUrl&)> processTrack;
                std::function<void(const QFileInfo&)> processPlaylist;

                processPlaylist = [&](const QFileInfo& fileInfo) {
                    QString absoluteFilePath(fileInfo.absoluteFilePath());
                    if (!contains(playlists, absoluteFilePath)) {
                        playlists.insert(std::move(absoluteFilePath));

                        const std::vector<PlaylistTrack> playlistTracks(PlaylistUtils::parsePlaylist(fileInfo.filePath()));
                        newTracks.reserve(newTracks.size() + playlistTracks.size());
                        for (const PlaylistTrack& playlistTrack : playlistTracks) {
                            if (playlistTrack.url.isLocalFile()) {
                                processTrack(playlistTrack.url);
                            } else {
                                newTracks.push_back(std::make_shared<QueueTrack>(playlistTrack.url,
                                                                                 playlistTrack.title,
                                                                                 playlistTrack.duration,
                                                                                 playlistTrack.artist,
                                                                                 playlistTrack.album,
                                                                                 false));
                            }
                        }
                    }
                };

                processTrack = [&](const QUrl& url) {
                    if (url.isLocalFile()) {
                        const QFileInfo fileInfo(url.path());
                        if (PlaylistUtils::isPlaylistExtension(fileInfo.suffix())) {
                            processPlaylist(fileInfo);
                        } else {
                            const auto found(oldTracks.find(url));
                            if (found == oldTracksEnd) {
                                if (fileInfo.isFile() && fileInfo.isReadable()) {
                                    newTracks.push_back(std::make_shared<QueueTrack>(url, QString()));
                                    const auto inserted(tracksToQuery.insert(url.path()));
                                    tracksToQueryMap.emplace(*inserted.first, newTracks.back().get());
                                } else {
                                    qWarning() << "File is not readable:" << fileInfo.filePath();
                                }
                            } else {
                                newTracks.push_back(found->second);
                            }
                        }
                    } else {
                        newTracks.push_back(std::make_shared<QueueTrack>(url, url.toString()));
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

                qInfo("Finished creating tracks: %lldms", static_cast<long long>(timer.elapsed()));

                return {std::move(newTracks),
                        std::move(tracksToQuery),
                        std::move(tracksToQueryMap)};
            }

            static void extractTrackInfos(std::unordered_multimap<QString, QueueTrack*> tracksToQueryMap)
            {
                if (tracksToQueryMap.empty()) {
                    return;
                }

                qInfo("Start extracting tags, count=%zu", tracksToQueryMap.size());

                QElapsedTimer timer;
                timer.start();

                QString prevFilePath;
                tagutils::Info info;
                QString artist;
                QString album;
                for (const auto& i : tracksToQueryMap) {
                    if (!qApp) {
                        break;
                    }

                    const QString& filePath = i.first;
                    if (filePath != prevFilePath) {
                        prevFilePath = filePath;

                        const QFileInfo fileInfo(filePath);
                        info = tagutils::getTrackInfo(filePath, fileutils::extensionFromSuffix(fileInfo.suffix())).value_or(tagutils::Info{});
                        if (info.title.isEmpty()) {
                            info.title = fileInfo.fileName();
                        }
                        info.artists.append(info.albumArtists);
                        info.artists.removeDuplicates();
                        artist = joinStrings(info.artists);
                        album = joinStrings(info.albums);
                    }
                    QueueTrack* track = i.second;
                    track->title = info.title;
                    track->duration = info.duration;
                    track->artist = std::move(artist);
                    track->album = std::move(album);
                }

                qInfo("Finished extracting tags: %lldms", static_cast<long long>(timer.elapsed()));
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

    QueueTrack::QueueTrack(const QUrl& url, const QString& title)
        : QueueTrack(url, title, -1, QString(), QString(), false)
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
        QObject::connect(this, &Queue::currentTrackChanged, this, [=] {
            const QueueTrack* track = mTracks.empty() ? nullptr : mTracks[static_cast<size_t>(mCurrentIndex)].get();
            if (track && track->url.isLocalFile()) {
                if (track->libraryMediaArt.isEmpty()) {
                    MediaArtUtils::instance()->getMediaArtForFile(track->url.path(),
                                                                  track->filteredSingleAlbum ? track->album : QString(),
                                                                  !track->directoryMediaArt.isEmpty());
                } else {
                    setCurrentMediaArt(track->libraryMediaArt, {}, {});
                }
            } else {
                setCurrentMediaArt({}, {}, {});
            }
        });

        QObject::connect(MediaArtUtils::instance(), &MediaArtUtils::gotMediaArtForFile, this, [=](const QString& filePath, const QString& libraryMediaArt, const QString& directoryMediaArt, const QByteArray& embeddedMediaArtData) {
            if (filePath == currentFilePath()) {
                const auto& track = mTracks[static_cast<size_t>(mCurrentIndex)].get();
                if (track->libraryMediaArt.isEmpty()) {
                    track->libraryMediaArt = libraryMediaArt;
                }
                if (track->directoryMediaArt.isEmpty()) {
                    track->directoryMediaArt = directoryMediaArt;
                }
                setCurrentMediaArt(track->libraryMediaArt,
                                   track->directoryMediaArt,
                                   embeddedMediaArtData);
            }
        });

        QObject::connect(LibraryUtils::instance(), &LibraryUtils::mediaArtChanged, this, [=]() {
            if (!mTracks.empty()) {
                for (auto& track : mTracks) {
                    track->libraryMediaArt.clear();
                    track->directoryMediaArt.clear();
                }
                setCurrentMediaArt({}, {}, {});
                const QueueTrack* track = mTracks.empty() ? nullptr : mTracks[static_cast<size_t>(mCurrentIndex)].get();
                if (track && track->url.isLocalFile()) {
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
                if (!mCurrentLibraryMediaArt.isEmpty()) {
                    return mCurrentLibraryMediaArt;
                }
                if (!mCurrentEmbeddedMediaArtData.isEmpty() && (!Settings::instance()->useDirectoryMediaArt() || mCurrentDirectoryMediaArt.isEmpty())) {
                    return QString::fromLatin1("image://%1/%2").arg(QueueImageProvider::providerId, track->url.toString());
                }
                return mCurrentDirectoryMediaArt;
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
        switch (mode) {
        case NoRepeat:
        case RepeatAll:
        case RepeatOne:
        {
            const auto newMode = static_cast<RepeatMode>(mode);
            if (newMode != mRepeatMode) {
                mRepeatMode = newMode;
                emit repeatModeChanged();
            }
            return;
        }
        }
        qWarning("Failed to convert value %d to RepeatMode", mode);
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


        std::unordered_map<QUrl, std::shared_ptr<QueueTrack>> oldTracks;
        oldTracks.reserve(mTracks.size());

        for (const auto& track : mTracks) {
            if (track->url.isLocalFile()) {
                oldTracks.emplace(track->url, track);
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

    void Queue::setCurrentMediaArt(const QString& libraryMediaArt, const QString& directoryMediaArt, const QByteArray& embeddedMediaArtData)
    {
        bool changed = false;
        if (libraryMediaArt != mCurrentLibraryMediaArt) {
            mCurrentLibraryMediaArt = libraryMediaArt;
            changed = true;
        }
        if (directoryMediaArt != mCurrentDirectoryMediaArt) {
            mCurrentDirectoryMediaArt = directoryMediaArt;
            changed = true;
        }
        if (embeddedMediaArtData != mCurrentEmbeddedMediaArtData) {
            mCurrentEmbeddedMediaArtData = embeddedMediaArtData;
            emit mediaArtDataChanged(mCurrentEmbeddedMediaArtData);
            changed = true;
        }
        if (changed) {
            emit mediaArtChanged();
        }
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
