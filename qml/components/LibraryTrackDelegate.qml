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
import Sailfish.Silica 1.0

BaseTrackDelegate {
    current: model.filePath === player.queue.currentFilePath
    showDuration: true
    menu: Component {
        ContextMenu {
            MenuItem {
                text: qsTr("Track information")
                onClicked: pageStack.push("TrackInfoPage.qml", { filePath: model.filePath })
            }

            AddToQueueMenuItem {
                onClicked: player.queue.addTrack(model.filePath)
            }

            AddToPlaylistMenuItem {
                onClicked: pageStack.push("AddToPlaylistPage.qml", { tracks: model.filePath })
            }
        }
    }

    onClicked: {
        if (selectionPanel.showPanel) {
            tracksProxyModel.select(model.index)
        } else {
            if (current) {
                if (!player.playing) {
                    player.play()
                }
            } else {
                player.queue.addTracks(tracksModel.getTracks(tracksProxyModel.sourceIndexes), true, model.index)
            }
        }
    }
}
