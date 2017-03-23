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

#include "queue.h"

#include "utils.h"

namespace unplayer
{

QueueTrack::QueueTrack(const QVariantMap &trackMap)
    : title(trackMap.value("title").toString()),
      url(trackMap.value("url").toString()),
      duration(trackMap.value("duration").toLongLong()),
      artist(trackMap.value("artist").toString()),
      album(trackMap.value("album").toString()),
      rawArtist(trackMap.value("rawArtist").toString()),
      rawAlbum(trackMap.value("rawAlbum").toString())
{

}

Queue::Queue(QObject *parent)
    : QObject(parent),
      m_currentIndex(-1),
      m_shuffle(false),
      m_repeat(false)
{

}

Queue::~Queue()
{
    qDeleteAll(m_tracks);
}

const QList<QueueTrack*>& Queue::tracks() const
{
    return m_tracks;
}

int Queue::currentIndex() const
{
    return m_currentIndex;
}

void Queue::setCurrentIndex(int index)
{
    m_currentIndex = index;
    emit currentIndexChanged();
}

QString Queue::currentTitle() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_tracks.length())
        return m_tracks.at(m_currentIndex)->title;
    return QString();
}

QString Queue::currentUrl() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_tracks.length())
        return m_tracks.at(m_currentIndex)->url;
    return QString();
}

int Queue::currentDuration() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_tracks.length())
        return m_tracks.at(m_currentIndex)->duration;
    return 0;
}

QString Queue::currentArtist() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_tracks.length())
        return m_tracks.at(m_currentIndex)->artist;
    return QString();
}

QString Queue::currentAlbum() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_tracks.length())
        return m_tracks.at(m_currentIndex)->album;
    return QString();
}

QUrl Queue::currentMediaArt() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_tracks.length()) {
        const QueueTrack *track = m_tracks.at(m_currentIndex);
        return Utils::mediaArt(track->rawArtist, track->rawAlbum, track->url);
    }
    return QUrl();
}

bool Queue::isShuffle() const
{
    return m_shuffle;
}

void Queue::setShuffle(bool shuffle)
{
    m_shuffle = shuffle;
    emit shuffleChanged();

    if (!shuffle)
        resetNotPlayedTracks();
}

bool Queue::isRepeat() const
{
    return m_repeat;
}

void Queue::setRepeat(bool repeat)
{
    m_repeat = repeat;
    emit repeatChanged();

    if (m_shuffle && !repeat)
        resetNotPlayedTracks();
}

void Queue::add(const QVariantList &trackList)
{
    if (!trackList.isEmpty()) {
        for (const QVariant &trackVariant : trackList) {
            QueueTrack *track = new QueueTrack(trackVariant.toMap());
            m_tracks.append(track);
            m_notPlayedTracks.append(track);
        }
    }
}

void Queue::remove(const QList<int> &indexes)
{
    if (indexes.size() == m_tracks.size()) {
        reset();
        return;
    }

    emit tracksRemoved(indexes);

    QueueTrack *currentTrack = m_tracks.at(m_currentIndex);
    bool trackChanged = false;
    QueueTrack *newTrack = nullptr;

    if (indexes.contains(m_currentIndex)) {
        trackChanged = true;
        int newIndex = -1;

        for (int i = m_currentIndex + 1, tracksCount = m_tracks.size(); i < tracksCount; i++) {
            if (!indexes.contains(i)) {
                newIndex = i;
                break;
            }
        }

        if (newIndex == -1) {
            for (int i = 0, max = m_currentIndex; i < max; i++) {
                if (!indexes.contains(i)) {
                    newIndex = i;
                    break;
                }
            }
        }

        if (newIndex != -1)
            newTrack = m_tracks.at(newIndex);
    }

    for (int i = 0, indexesCount = indexes.size(); i < indexesCount; i++) {
        QueueTrack *track = m_tracks.takeAt(indexes.at(i) - i);
        m_notPlayedTracks.removeOne(track);
        delete track;
    }

    if (trackChanged) {
        if (newTrack)
            setCurrentIndex(m_tracks.indexOf(newTrack));
        else
            setCurrentIndex(-1);
        emit currentTrackChanged();
    } else {
        if (m_currentIndex >= indexes.first())
            setCurrentIndex(m_tracks.indexOf(currentTrack));
    }
}

void Queue::clear()
{
    qDeleteAll(m_tracks);
    m_tracks.clear();
    m_notPlayedTracks.clear();
}

bool Queue::hasUrl(const QString &url)
{
    for (const QueueTrack *track : m_tracks) {
        if (track->url == url)
            return true;
    }

    return false;
}

void Queue::next()
{
    if (m_shuffle) {
        if (m_notPlayedTracks.length() == 1)
            resetNotPlayedTracks();
        m_notPlayedTracks.removeOne(m_tracks.at(m_currentIndex));
        setCurrentIndex(m_tracks.indexOf(m_notPlayedTracks.at(qrand() % m_notPlayedTracks.length())));
    } else {
        if (m_currentIndex == m_tracks.length() - 1)
            setCurrentIndex(0);
        else
            setCurrentIndex(m_currentIndex + 1);
    }

    emit currentTrackChanged();
}

bool Queue::nextOnEos()
{
    if (m_shuffle) {
        QueueTrack *track = m_tracks.at(m_currentIndex);
        m_notPlayedTracks.removeOne(track);
        if (m_notPlayedTracks.length() == 0) {
            if (m_repeat) {
                resetNotPlayedTracks();
                m_notPlayedTracks.removeOne(track);
            } else {
                return false;
            }
        }
        setCurrentIndex(m_tracks.indexOf(m_notPlayedTracks.at(qrand() % m_notPlayedTracks.length())));
    } else {
        if (m_currentIndex == m_tracks.length() - 1) {
            if (m_repeat)
                setCurrentIndex(0);
            else
                return false;
        } else {
            setCurrentIndex(m_currentIndex + 1);
        }
    }

    emit currentTrackChanged();

    return true;
}

void Queue::previous()
{
    if (m_shuffle)
        return;

    if (m_currentIndex == 0)
        setCurrentIndex(m_tracks.length() - 1);
    else
        setCurrentIndex(m_currentIndex - 1);

    emit currentTrackChanged();
}

void Queue::setCurrentToFirstIfNeeded()
{
    if (m_currentIndex == -1) {
        setCurrentIndex(0);
        emit currentTrackChanged();
    }
}

void Queue::resetNotPlayedTracks()
{
    m_notPlayedTracks = m_tracks;
}

void Queue::reset()
{
    clear();
    setCurrentIndex(-1);
    emit currentTrackChanged();
}

}
