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

import QtQuick 2.2
import Sailfish.Silica 1.0

import "../components"

BaseTrackDelegate {
    current: model.url === player.queue.currentUrl
    menu: Component {
        ContextMenu {
            MenuItem {
                text: qsTr("Track information")
                onClicked: pageStack.push("../pages/TrackInfoPage.qml", { trackUrl: model.url })
            }

            AddToQueueMenuItem {
                onClicked: {
                    player.queue.add([listView.model.sourceModel.get(listView.model.sourceIndex(model.index))])
                    player.queue.setCurrentToFirstIfNeeded()
                }
            }

            AddToPlaylistMenuItem {
                onClicked: pageStack.push("../pages/AddToPlaylistPage.qml", { tracks: model.url })
            }
        }
    }

    onClicked: {
        if (selectionPanel.showPanel) {
            listView.model.select(model.index)
        } else {
            if (current) {
                if (!player.playing)
                    player.play()
                return
            }

            player.queue.clear()
            player.queue.addTracks(listView.model.getTracks())
            player.queue.currentIndex = model.index
            player.queue.currentTrackChanged()
            player.play()
        }
    }
}
