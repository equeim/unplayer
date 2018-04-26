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

import QtQuick 2.2
import Sailfish.Silica 1.0

import harbour.unplayer 0.1 as Unplayer

BaseTrackDelegate {
    current: model.filePath === Unplayer.Player.queue.currentFilePath
    showDuration: true
    menu: Component {
        ContextMenu {
            MenuItem {
                text: qsTranslate("unplayer", "Track information")
                onClicked: pageStack.push("TrackInfoPage.qml", { filePath: model.filePath })
            }

            MenuItem {
                text: qsTranslate("unplayer", "Add to queue")
                onClicked: Unplayer.Player.queue.addTrack(model.filePath)
            }

            MenuItem {
                text: qsTranslate("unplayer", "Add to playlist")
                onClicked: pageStack.push("AddToPlaylistPage.qml", { tracks: model.filePath })
            }
        }
    }

    onClicked: {
        if (selectionPanel.showPanel) {
            tracksProxyModel.select(model.index)
        } else {
            if (current) {
                if (!Unplayer.Player.playing) {
                    Unplayer.Player.play()
                }
            } else {
                Unplayer.Player.queue.addTracks(tracksModel.getTracks(tracksProxyModel.sourceIndexes), true, model.index)
            }
        }
    }
}
