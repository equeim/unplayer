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

#include <QVariantMap>

#include "utils.h"

namespace unplayer
{
    QueueTrack::QueueTrack(const QString& url,
                           const QString& title,
                           long long duration,
                           const QString& artist,
                           const QString& album,
                           const QString& rawArtist,
                           const QString& rawAlbum)
        : url(url),
          title(title),
          duration(duration),
          artist(artist),
          album(album),
          rawArtist(rawArtist),
          rawAlbum(rawAlbum)
    {
    }

    Queue::Queue(QObject* parent)
        : QObject(parent),
          mCurrentIndex(-1),
          mShuffle(false),
          mRepeat(false)
    {
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

    QString Queue::currentUrl() const
    {
        if (mCurrentIndex >= 0 && mCurrentIndex < mTracks.size()) {
            return mTracks.at(mCurrentIndex)->url;
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

    QUrl Queue::currentMediaArt() const
    {
        if (mCurrentIndex >= 0 && mCurrentIndex < mTracks.size()) {
            const QueueTrack* track = mTracks.at(mCurrentIndex).get();
            return Utils::mediaArt(track->rawArtist, track->rawAlbum, track->url);
        }
        return QUrl();
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

    bool Queue::isRepeat() const
    {
        return mRepeat;
    }

    void Queue::setRepeat(bool repeat)
    {
        if (repeat != mRepeat) {
            mRepeat = repeat;
            emit repeatChanged();
            if (mShuffle && !repeat) {
                resetNotPlayedTracks();
            }
        }
    }

    void Queue::addTracks(const QVariantList& tracks)
    {
        const int start = mTracks.size();

        for (const QVariant& trackVariant : tracks) {
            const QVariantMap trackMap(trackVariant.toMap());
            const auto track(std::make_shared<QueueTrack>(trackMap.value(QStringLiteral("url")).toString(),
                                                          trackMap.value(QStringLiteral("title")).toString(),
                                                          trackMap.value(QStringLiteral("duration")).toLongLong(),
                                                          trackMap.value(QStringLiteral("artist")).toString(),
                                                          trackMap.value(QStringLiteral("album")).toString(),
                                                          trackMap.value(QStringLiteral("rawArtist")).toString(),
                                                          trackMap.value(QStringLiteral("rawAlbum")).toString()));
            mTracks.append(track);
            mNotPlayedTracks.append(track);
        }

        emit tracksAdded(start);
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

    void Queue::removeTracks(QList<int> indexes)
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
    }

    bool Queue::hasUrl(const QString& url)
    {
        for (const std::shared_ptr<QueueTrack>& track : mTracks) {
            if (track->url == url) {
                return true;
            }
        }
        return false;
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
        if (mShuffle) {
            const std::shared_ptr<QueueTrack>& track = mTracks.at(mCurrentIndex);
            mNotPlayedTracks.removeOne(track);
            if (mNotPlayedTracks.size() == 0) {
                if (mRepeat) {
                    resetNotPlayedTracks();
                    mNotPlayedTracks.removeOne(track);
                } else {
                    return;
                }
            }
            setCurrentIndex(mTracks.indexOf(mNotPlayedTracks.at(qrand() % mNotPlayedTracks.size())));
        } else {
            if (mCurrentIndex == (mTracks.size() - 1)) {
                if (mRepeat) {
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
