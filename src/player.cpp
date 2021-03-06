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

#include "player.h"

#include <QCoreApplication>
#include <QDebug>
#include <QMediaContent>
#include <QUrl>

#include <MprisPlayer>

#ifdef UNPLAYER_SAILFISHOS
#include <notification.h>
#endif

#include "queue.h"
#include "settings.h"

namespace unplayer
{
    namespace
    {
        Mpris::LoopStatus loopStatus(Queue::RepeatMode mode) {
            switch (mode) {
            case Queue::NoRepeat:
                return Mpris::None;
            case Queue::RepeatAll:
                return Mpris::Playlist;
            case Queue::RepeatOne:
                return Mpris::Track;
            default:
                return Mpris::None;
            }
        }

        Queue::RepeatMode repeatMode(Mpris::LoopStatus status) {
            switch (status) {
            case Mpris::None:
                return Queue::NoRepeat;
            case Mpris::Playlist:
                return Queue::RepeatAll;
            case Mpris::Track:
                return Queue::RepeatOne;
            default:
                return Queue::NoRepeat;
            }
        }

#ifdef UNPLAYER_SAILFISHOS
        void showErrorNotification(QMediaPlayer::Error error, const QString& errorString)
        {
            QString summary;
            if (error == QMediaPlayer::FormatError) {
                summary = qApp->translate("unplayer", "File format is not supported");
            } else {
                summary = qApp->translate("unplayer", "Error playing file");
            }
            Notification notification;
            notification.setIsTransient(true);
            notification.setPreviewSummary(summary);
            notification.setPreviewBody(errorString);
            notification.publish();
        }
#endif
    }

    Player* Player::instance()
    {
        static const auto p = new Player(qApp);
        return p;
    }

    bool Player::isPlaying() const
    {
        return (state() == PlayingState);
    }

    bool Player::stopAfterEos() const
    {
        return mStopAfterEos;
    }

    void Player::setStopAfterEos(bool stop)
    {
        if (stop != mStopAfterEos) {
            mStopAfterEos = stop;
            emit stopAfterEosChanged();
        }
    }

    Queue* Player::queue() const
    {
        return mQueue;
    }

    void Player::saveState() const
    {
        QStringList tracks;
        tracks.reserve(static_cast<int>(mQueue->tracks().size()));
        for (const auto& track : mQueue->tracks()) {
            tracks.push_back(track->url.toString());
        }
        Settings::instance()->savePlayerState(tracks,
                                              mQueue->currentIndex(),
                                              mQueue->isShuffle(),
                                              mQueue->repeatMode(),
                                              position(),
                                              mStopAfterEos);
    }

    void Player::restoreState()
    {
        mQueue->setShuffle(Settings::instance()->shuffle());
        mQueue->setRepeatMode(Settings::instance()->repeatMode());
        mStopAfterEos = Settings::instance()->stopAfterEos();
        const auto tracks(Settings::instance()->queueTracks());
        if (!tracks.isEmpty()) {
            mRestoringTracks = true;
            mQueue->addTracksFromUrls(tracks, true, Settings::instance()->queuePosition());
        }
    }

    Player::Player(QObject* parent)
        : QMediaPlayer(parent),
          mQueue(new Queue(this)),
          mSettingNewTrack(false),
          mRestoringTracks(false),
          mStopAfterEos(false),
          mStoppedAfterEos(false)
    {
        auto mpris = new MprisPlayer(this);
        mpris->setServiceName(QLatin1String("unplayer"));
        mpris->setCanControl(true);

        mpris->setLoopStatus(loopStatus(mQueue->repeatMode()));
        QObject::connect(mQueue, &Queue::repeatModeChanged, this, [=]() {
            mpris->setLoopStatus(loopStatus(mQueue->repeatMode()));
        });

        mpris->setShuffle(mQueue->isShuffle());
        QObject::connect(mQueue, &Queue::shuffleChanged, this, [=]() {
            mpris->setShuffle(mQueue->isShuffle());
        });

        QObject::connect(this, &Player::stateChanged, this, [=](State newState) {
            qInfo() << "stateChanged" << newState;
            if (mSettingNewTrack) {
                return;
            }

            static State oldState = StoppedState;
            if (newState != oldState) {
                if (newState == PlayingState || oldState == PlayingState) {
                    emit playingChanged();
                }
                oldState = newState;

                switch (newState) {
                case StoppedState:
                    mpris->setPlaybackStatus(Mpris::Stopped);
                    break;
                case PlayingState:
                    mpris->setPlaybackStatus(Mpris::Playing);
                    if (mStoppedAfterEos) {
                        mStoppedAfterEos = false;
                        mQueue->nextOnEos();
                    }
                    break;
                case PausedState:
                    mpris->setPlaybackStatus(Mpris::Paused);
                }
            }
        });

        QObject::connect(this, &Player::mediaStatusChanged, this, [=](MediaStatus status) {
            qInfo() << "mediaStatusChanged" << status;
            if (status == EndOfMedia) {
                if (mStopAfterEos) {
                    mStoppedAfterEos = true;
                } else {
                    mQueue->nextOnEos();
                }
            } else if (status == InvalidMedia) {
                qWarning("Invalid media");
            }
        });

        QObject::connect(this, static_cast<void(Player::*)(Error)>(&Player::error), this, [=](Error error) {
            qWarning() << "error" << error << errorString();
#ifdef UNPLAYER_SAILFISHOS
            showErrorNotification(error, errorString());
#endif
        });

        QObject::connect(this, &Player::positionChanged, this, [=](qint64 position) {
            mpris->setPosition(position * 1000);
        });

        QObject::connect(mQueue, &Queue::currentTrackChanged, this, [=](bool setAsCurrentWasSet) {
            qInfo() << "currentTrackChanged, setAsCurrentWasSet =" << setAsCurrentWasSet;
            qInfo("currentIndex = %d", mQueue->currentIndex());
            if (mQueue->currentIndex() == -1) {
                setMedia(QMediaContent());

                mpris->setCanPlay(false);
                mpris->setCanPause(false);
                mpris->setCanGoNext(false);
                mpris->setCanGoPrevious(false);
                mpris->setCanSeek(false);
                mpris->setMetadata(QVariantMap());
            } else {
                const QueueTrack* track = mQueue->tracks()[static_cast<size_t>(mQueue->currentIndex())].get();
                qInfo() << "track =" << track->url;

                mSettingNewTrack = true;
                setMedia(track->url);
                mSettingNewTrack = false;

                if (mRestoringTracks) {
                    if (setAsCurrentWasSet) {
                        // Restore position only if we set correct track as current
                        setPosition(Settings::instance()->playerPosition());
                    }
                    mRestoringTracks = false;
                } else {
                    play();
                }

                mpris->setCanPlay(true);
                mpris->setCanPause(true);
                mpris->setCanGoNext(true);
                mpris->setCanGoPrevious(true);
                mpris->setCanSeek(true);
                mpris->setMetadata({{Mpris::metadataToString(Mpris::TrackId), track->getTrackId()},
                                    {Mpris::metadataToString(Mpris::Title), track->title},
                                    {Mpris::metadataToString(Mpris::Length), track->duration * 1000000LL},
                                    {Mpris::metadataToString(Mpris::Artist), track->artist},
                                    {Mpris::metadataToString(Mpris::Album), track->album}});
            }
        });

        QObject::connect(mQueue, &Queue::addingTracksChanged, this, [=]() {
            if (!mQueue->isAddingTracks()) {
                // If we are restoring state but no tracks are actually added to queue (e.g. because they all have been removed)
                // currentTrackChanged() won't be emitted so we need to reset mRestoringTracks here too.
                mRestoringTracks = false;
            }
        });

        QObject::connect(mpris, &MprisPlayer::playRequested, this, &Player::play);
        QObject::connect(mpris, &MprisPlayer::pauseRequested, this, &Player::pause);
        QObject::connect(mpris, &MprisPlayer::nextRequested, mQueue, &Queue::next);
        QObject::connect(mpris, &MprisPlayer::previousRequested, mQueue, &Queue::previous);
        QObject::connect(mpris, &MprisPlayer::setPositionRequested, this, [=](const QDBusObjectPath& trackId, qint64 position) {
            if (state() != StoppedState &&
                    mQueue->currentIndex() != -1 &&
                    trackId.path() == mQueue->tracks()[static_cast<size_t>(mQueue->currentIndex())]->getTrackId()) {
                setPosition(position / 1000);
            }
        });
        QObject::connect(mpris, &MprisPlayer::seekRequested, this, [=](qint64 offset) {
            if (state() != StoppedState) {
                setPosition(position() - (offset / 1000));
            }
        });
        QObject::connect(mpris, &MprisPlayer::loopStatusRequested, this, [=](Mpris::LoopStatus status) {
            if (mpris->canControl()) {
                mQueue->setRepeatMode(repeatMode(status));
            }
        });
        QObject::connect(mpris, &MprisPlayer::shuffleRequested, this, [=](bool shuffle) {
            if (mpris->canControl()) {
                mQueue->setShuffle(shuffle);
            }
        });
    }
}
