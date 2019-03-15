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

#ifndef UNPLAYER_PLAYER_H
#define UNPLAYER_PLAYER_H

#include <QMediaPlayer>

namespace unplayer
{
    class Queue;

    class Player final : public QMediaPlayer
    {
        Q_OBJECT
        Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
        Q_PROPERTY(bool stopAfterEos READ stopAfterEos WRITE setStopAfterEos NOTIFY stopAfterEosChanged)
        Q_PROPERTY(unplayer::Queue* queue READ queue CONSTANT)
    public:
        static Player* instance();

        bool isPlaying() const;

        bool stopAfterEos() const;
        void setStopAfterEos(bool stop);

        Queue* queue() const;

        Q_INVOKABLE void saveState() const;
        Q_INVOKABLE void restoreState();

    private:
        Player(QObject* parent);

        Queue* mQueue;
        bool mSettingNewTrack;

        bool mRestoringTracks;

        bool mStopAfterEos;
        bool mStoppedAfterEos;

    signals:
        void playingChanged();
        void stopAfterEosChanged();
    };
}

#endif // UNPLAYER_PLAYER_H
