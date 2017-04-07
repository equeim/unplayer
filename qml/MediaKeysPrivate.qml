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

import QtQuick 2.2
import Sailfish.Media 1.0

import harbour.unplayer 0.1 as Unplayer

Item {
    MediaKey {
        enabled: true
        key: Qt.Key_MediaTogglePlayPause
        onReleased: Unplayer.Player.playing ? Unplayer.Player.pause() : Unplayer.Player.play()
    }
    MediaKey {
        enabled: true
        key: Qt.Key_MediaPlay
        onReleased: Unplayer.Player.play()
    }
    MediaKey {
        enabled: true
        key: Qt.Key_MediaPause
        onReleased: Unplayer.Player.pause()
    }
    MediaKey {
        enabled: true
        key: Qt.Key_MediaStop
        onReleased: Unplayer.Player.stop()
    }
    MediaKey {
        enabled: true
        key: Qt.Key_MediaNext
        onReleased: Unplayer.Player.queue.next()
    }
    MediaKey {
        enabled: true
        key: Qt.Key_MediaPrevious
        onReleased: Unplayer.Player.queue.previous()
    }
    MediaKey {
        enabled: true
        key: Qt.Key_ToggleCallHangup
        onReleased: Unplayer.Player.playing ? Unplayer.Player.pause() : Unplayer.Player.play()
    }
}
