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

Item {
    MediaKey {
        enabled: true
        key: Qt.Key_MediaTogglePlayPause
        onReleased: player.playing ? player.pause() : player.play()
    }
    MediaKey {
        enabled: true
        key: Qt.Key_MediaPlay
        onReleased: player.play()
    }
    MediaKey {
        enabled: true
        key: Qt.Key_MediaPause
        onReleased: player.pause()
    }
    MediaKey {
        enabled: true
        key: Qt.Key_MediaStop
        onReleased: player.stop()
    }
    MediaKey {
        enabled: true
        key: Qt.Key_MediaNext
        onReleased: player.queue.next()
    }
    MediaKey {
        enabled: true
        key: Qt.Key_MediaPrevious
        onReleased: player.queue.previous()
    }
    MediaKey {
        enabled: true
        key: Qt.Key_ToggleCallHangup
        onReleased: player.playing ? player.pause() : player.play()
    }
}
