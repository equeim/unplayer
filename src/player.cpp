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

#include "player.h"

#include <QCoreApplication>
#include <QDebug>
#include <QUrl>

#include <MprisPlayer>

#include "queue.h"
#include "settings.h"

namespace unplayer
{
    namespace
    {
        Player* instancePointer = nullptr;
    }

    Player* Player::instance()
    {
        if (!instancePointer) {
            instancePointer = new Player(qApp);
        }
        return instancePointer;
    }

    Player::~Player()
    {
        saveState();
    }

    bool Player::isPlaying() const
    {
        return (state() == PlayingState);
    }

    Queue* Player::queue() const
    {
        return mQueue;
    }

    void Player::saveState() const
    {
        QStringList tracks;
        tracks.reserve(mQueue->tracks().size());
        for (const auto& track : mQueue->tracks()) {
            tracks.append(track->filePath);
        }
        Settings::instance()->savePlayerState(tracks, mQueue->currentIndex(), position());
    }

    void Player::restoreState()
    {
        mRestoringState = true;
        mQueue->addTracks(Settings::instance()->queueTracks(), true, Settings::instance()->queuePosition());
    }

    Player::Player(QObject* parent)
        : QMediaPlayer(parent),
          mQueue(new Queue(this)),
          mSettingNewTrack(false),
          mRestoringState(false)
    {
        auto mpris = new MprisPlayer(this);
        mpris->setServiceName(QLatin1String("unplayer"));
        mpris->setCanControl(true);

        QObject::connect(this, &Player::stateChanged, [=](State newState) {
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
                    break;
                case PausedState:
                    mpris->setPlaybackStatus(Mpris::Paused);
                }
            }
        });

        QObject::connect(this, &Player::mediaStatusChanged, this, [=](MediaStatus status) {
            if (status == EndOfMedia) {
                mQueue->nextOnEos();
            } else if (status == InvalidMedia) {
                qWarning() << error() << errorString();
            }
        });

        QObject::connect(mQueue, &Queue::currentTrackChanged, this, [=]() {
            if (mQueue->currentIndex() == -1) {
                setMedia(QMediaContent());

                mpris->setCanPlay(false);
                mpris->setCanPause(false);
                mpris->setCanGoNext(false);
                mpris->setCanGoPrevious(false);
                mpris->setMetadata(QVariantMap());
            } else {
                const QueueTrack* track = mQueue->tracks().at(mQueue->currentIndex()).get();

                mSettingNewTrack = true;
                setMedia(QUrl::fromLocalFile(track->filePath));
                mSettingNewTrack = false;

                if (mRestoringState) {
                    setPosition(Settings::instance()->playerPosition());
                    mRestoringState = false;
                } else {
                    play();
                }

                mpris->setCanPlay(true);
                mpris->setCanPause(true);
                mpris->setCanGoNext(true);
                mpris->setCanGoPrevious(true);
                mpris->setMetadata({{Mpris::metadataToString(Mpris::Title), track->title},
                                    {Mpris::metadataToString(Mpris::Artist), track->artist},
                                    {Mpris::metadataToString(Mpris::Album), track->album}});
            }
        });

        QObject::connect(mpris, &MprisPlayer::playRequested, this, &Player::play);
        QObject::connect(mpris, &MprisPlayer::pauseRequested, this, &Player::pause);
        QObject::connect(mpris, &MprisPlayer::nextRequested, mQueue, &Queue::next);
        QObject::connect(mpris, &MprisPlayer::previousRequested, mQueue, &Queue::previous);
    }
}
