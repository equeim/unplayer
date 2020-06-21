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

#ifndef UNPLAYER_QUEUE_H
#define UNPLAYER_QUEUE_H

#include <memory>
#include <mutex>
#include <vector>

#include <QObject>
#include <QQuickImageProvider>
#include <QPixmap>
#include <QStringList>
#include <QUrl>

#include "librarytrack.h"

namespace unplayer
{
    struct QueueTrack
    {
        explicit QueueTrack(const QUrl& url,
                            const QString& title,
                            int duration,
                            const QString& artist,
                            const QString& album,
                            bool filteredSingleAlbum);

        explicit QueueTrack(const QUrl& url,
                            const QString& title);

        inline const QString& getTrackId() const
        {
            if (mTrackId.isEmpty()) {
                initTrackId();
            }
            return mTrackId;
        }

        QUrl url;
        QString title;
        int duration;
        QString artist;
        QString album;
        bool filteredSingleAlbum;

        QString libraryMediaArt;
        QString directoryMediaArt;

    private:
        void initTrackId() const;
        mutable QString mTrackId;
    };

    class Queue final : public QObject
    {
        Q_OBJECT

        Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

        Q_PROPERTY(QUrl currentUrl READ currentUrl NOTIFY currentTrackChanged)
        Q_PROPERTY(bool currentIsLocalFile READ isCurrentLocalFile NOTIFY currentTrackChanged)
        Q_PROPERTY(QString currentFilePath READ currentFilePath NOTIFY currentTrackChanged)
        Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY currentTrackChanged)
        Q_PROPERTY(QString currentArtist READ currentArtist NOTIFY currentTrackChanged)
        Q_PROPERTY(QString currentAlbum READ currentAlbum NOTIFY currentTrackChanged)
        Q_PROPERTY(QUrl currentMediaArt READ currentMediaArt NOTIFY mediaArtChanged)

        Q_PROPERTY(bool shuffle READ isShuffle WRITE setShuffle NOTIFY shuffleChanged)
        Q_PROPERTY(RepeatMode repeatMode READ repeatMode NOTIFY repeatModeChanged)

        Q_PROPERTY(bool addingTracks READ isAddingTracks NOTIFY addingTracksChanged)
    public:
        enum RepeatMode
        {
            NoRepeat,
            RepeatAll,
            RepeatOne
        };
        Q_ENUM(RepeatMode)

        explicit Queue(QObject* parent);

        const std::vector<std::shared_ptr<QueueTrack>>& tracks() const;

        int currentIndex() const;
        void setCurrentIndex(int index);

        QUrl currentUrl() const;
        bool isCurrentLocalFile() const;
        QString currentFilePath() const;
        QString currentTitle() const;
        QString currentArtist() const;
        QString currentAlbum() const;
        QString currentMediaArt() const;

        bool isShuffle() const;
        void setShuffle(bool shuffle);

        RepeatMode repeatMode() const;
        Q_INVOKABLE void changeRepeatMode();
        void setRepeatMode(int mode);

        bool isAddingTracks() const;

        Q_INVOKABLE void addTracksFromUrls(const QStringList& trackUrls, bool clearQueue = false, int setAsCurrent = -1);
        Q_INVOKABLE void addTrackFromUrl(const QString& trackUrl);
        Q_INVOKABLE void addTracksFromLibrary(const std::vector<unplayer::LibraryTrack>& libraryTracks, bool clearQueue = false, int setAsCurrent = -1);
        Q_INVOKABLE void addTrackFromLibrary(const unplayer::LibraryTrack& libraryTrack, bool clearQueue = false, int setAsCurrent = -1);

        Q_INVOKABLE unplayer::LibraryTrack getTrack(int index) const;
        Q_INVOKABLE std::vector<unplayer::LibraryTrack> getTracks(const std::vector<int>& indexes) const;
        Q_INVOKABLE bool hasLocalFileForTracks(const std::vector<int>& indexes) const;
        Q_INVOKABLE QStringList getTrackPaths(const std::vector<int>& indexes) const;

        Q_INVOKABLE void removeTrack(int index);
        Q_INVOKABLE void removeTracks(std::vector<int> indexes);
        Q_INVOKABLE void clear(bool emitAbout = true);

        Q_INVOKABLE void next();
        void nextOnEos();
        Q_INVOKABLE void previous();

        Q_INVOKABLE void setCurrentToFirstIfNeeded();
        Q_INVOKABLE void resetNotPlayedTracks();

    private:
        void reset();
        void addingTracksCallback(std::vector<std::shared_ptr<QueueTrack>>&& tracks, int setAsCurrent, const QUrl& setAsCurrentUrl);
        void setCurrentMediaArt(const QString& libraryMediaArt, const QString& directoryMediaArt, const QByteArray& embeddedMediaArtData);

    private:
        std::vector<std::shared_ptr<QueueTrack>> mTracks;
        std::vector<const QueueTrack*> mNotPlayedTracks;

        int mCurrentIndex;
        bool mShuffle;
        RepeatMode mRepeatMode;

        QString mCurrentLibraryMediaArt;
        QString mCurrentDirectoryMediaArt;
        QByteArray mCurrentEmbeddedMediaArtData;

        bool mAddingTracks;
    signals:
        void currentTrackChanged(bool setAsCurrentWasSet = false);

        void mediaArtDataChanged(const QByteArray& mediaArtData);
        void mediaArtChanged();

        void currentIndexChanged();
        void shuffleChanged();
        void repeatModeChanged();

        void tracksAboutToBeAdded(int count);
        void tracksAdded();

        void trackAboutToBeRemoved(int index);
        void trackRemoved();

        void aboutToBeCleared();
        void cleared();

        void addingTracksChanged();
    };

    class QueueImageProvider final : public QQuickImageProvider, QObject
    {
    public:
        static const QLatin1String providerId;
        explicit QueueImageProvider(const Queue* queue);
        QueueImageProvider(const QueueImageProvider& other) = delete;
        QueueImageProvider& operator=(const QueueImageProvider& other) = delete;
        QPixmap requestPixmap(const QString& id, QSize*, const QSize& requestedSize) override;
    private:
        std::mutex mMutex;
        std::mutex mLoadingMutex;
        QPixmap mPixmap;
        QByteArray mMediaArtData;
    };
}

#endif // UNPLAYER_QUEUE_H
