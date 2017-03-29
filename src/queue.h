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

#ifndef UNPLAYER_QUEUE_H
#define UNPLAYER_QUEUE_H

#include <memory>

#include <QObject>
#include <QUrl>

namespace unplayer
{
    struct QueueTrack
    {
        explicit QueueTrack(const QString& url,
                            const QString& title,
                            long long duration,
                            const QString& artist,
                            const QString& album,
                            const QString& rawArtist,
                            const QString& rawAlbum);

        QString url;
        QString title;
        long long duration;
        QString artist;
        QString album;
        QString rawArtist;
        QString rawAlbum;
    };

    class Queue : public QObject
    {
        Q_OBJECT

        Q_ENUMS(RepeatMode)

        Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

        Q_PROPERTY(QString currentUrl READ currentUrl NOTIFY currentTrackChanged)
        Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY currentTrackChanged)
        Q_PROPERTY(QString currentArtist READ currentArtist NOTIFY currentTrackChanged)
        Q_PROPERTY(QString currentAlbum READ currentAlbum NOTIFY currentTrackChanged)
        Q_PROPERTY(QUrl currentMediaArt READ currentMediaArt NOTIFY currentTrackChanged)

        Q_PROPERTY(bool shuffle READ isShuffle WRITE setShuffle NOTIFY shuffleChanged)
        Q_PROPERTY(RepeatMode repeatMode READ repeatMode NOTIFY repeatModeChanged)
    public:
        enum RepeatMode
        {
            NoRepeat,
            RepeatAll,
            RepeatOne
        };

        explicit Queue(QObject* parent);

        const QList<std::shared_ptr<QueueTrack>>& tracks() const;

        int currentIndex() const;
        void setCurrentIndex(int index);

        QString currentUrl() const;
        QString currentTitle() const;
        QString currentArtist() const;
        QString currentAlbum() const;
        QUrl currentMediaArt() const;

        bool isShuffle() const;
        void setShuffle(bool shuffle);

        RepeatMode repeatMode() const;
        Q_INVOKABLE void changeRepeatMode();

        Q_INVOKABLE void addTracks(const QVariantList& tracks);
        Q_INVOKABLE void removeTrack(int index);
        Q_INVOKABLE void removeTracks(QList<int> indexes);
        Q_INVOKABLE void clear();

        Q_INVOKABLE bool hasUrl(const QString& url);

        Q_INVOKABLE void next();
        void nextOnEos();
        Q_INVOKABLE void previous();

        Q_INVOKABLE void setCurrentToFirstIfNeeded();
        Q_INVOKABLE void resetNotPlayedTracks();

    private:
        void reset();

    private:
        QList<std::shared_ptr<QueueTrack>> mTracks;
        QList<std::shared_ptr<QueueTrack>> mNotPlayedTracks;

        int mCurrentIndex;
        bool mShuffle;
        RepeatMode mRepeatMode;
    signals:
        void currentTrackChanged();

        void currentIndexChanged();
        void shuffleChanged();
        void repeatModeChanged();

        void tracksAdded(int start);
        void trackRemoved(int index);
        void tracksRemoved(const QList<int>& indexes);
        void cleared();
    };
}

#endif // UNPLAYER_QUEUE_H
