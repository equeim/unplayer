/*
 * Unplayer
 * Copyright (C) 2015 Alexey Rochev <equeim@gmail.com>
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

#include <QObject>
#include <QVariantMap>
#include <QUrl>

namespace Unplayer
{

struct QueueTrack
{
    QueueTrack(const QVariantMap &trackMap);
    QString title;
    QString url;
    qint64 duration;
    QString artist;
    QString album;

    QString rawArtist;
    QString rawAlbum;
};

class Queue : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY currentTrackChanged)
    Q_PROPERTY(QString currentUrl READ currentUrl NOTIFY currentTrackChanged)
    Q_PROPERTY(int currentDuration READ currentDuration NOTIFY currentTrackChanged)
    Q_PROPERTY(QString currentArtist READ currentArtist NOTIFY currentTrackChanged)
    Q_PROPERTY(QString currentAlbum READ currentAlbum NOTIFY currentTrackChanged)
    Q_PROPERTY(QUrl currentMediaArt READ currentMediaArt NOTIFY currentTrackChanged)

    Q_PROPERTY(bool shuffle READ isShuffle WRITE setShuffle NOTIFY shuffleChanged)
    Q_PROPERTY(bool repeat READ isRepeat WRITE setRepeat NOTIFY repeatChanged)
public:
    Queue(QObject *parent = 0);
    ~Queue();

    const QList<QueueTrack*>& tracks() const;

    int currentIndex() const;
    void setCurrentIndex(int index);

    QString currentTitle() const;
    QString currentUrl() const;
    int currentDuration() const;
    QString currentArtist() const;
    QString currentAlbum() const;
    QUrl currentMediaArt() const;

    bool isShuffle() const;
    void setShuffle(bool shuffle);

    bool isRepeat() const;
    void setRepeat(bool repeat);

    Q_INVOKABLE void add(const QVariantList &trackList);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();

    Q_INVOKABLE bool hasUrl(const QString &url);

    Q_INVOKABLE void next();
    bool nextOnEos();
    Q_INVOKABLE void previous();

    Q_INVOKABLE void resetNotPlayedTracks();
private:
    QList<QueueTrack*> m_tracks;
    QList<QueueTrack*> m_notPlayedTracks;

    QString m_hash;
    int m_currentIndex;
    bool m_shuffle;
    bool m_repeat;
signals:
    void currentTrackChanged();

    void currentIndexChanged();
    void shuffleChanged();
    void repeatChanged();

    void trackRemoved(int index);
};

}

#endif // UNPLAYER_QUEUE_H
