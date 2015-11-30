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

import harbour.unplayer 0.1 as Unplayer

import "../components"
import "../models"

Page {
    Connections {
        target: Unplayer.PlaylistUtils
        onPlaylistsChanged: playlistsModel.reload()
    }

    SearchListView {
        id: listView

        anchors.fill: parent
        headerTitle: qsTr("Playlists")
        delegate: MediaContainerListItem {
            title: Theme.highlightText(model.title, listView.searchFieldText.trim(), Theme.highlightColor)
            description: model.tracksCount === undefined ? String() :
                                                           qsTr("%n track(s)", String(), model.tracksCount)
            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Remove")
                    onClicked: remorseAction(qsTr("Removing"), function() {
                        Unplayer.PlaylistUtils.removePlaylist(model.url)
                    })
                }
            }

            onClicked: pageStack.push("PlaylistPage.qml", {
                                          title: model.title,
                                          playlistUrl: model.url
                                      })
        }
        model: Unplayer.FilterProxyModel {
            id: playlistsProxyModel

            filterRoleName: "title"
            sourceModel: PlaylistsModel {
                id: playlistsModel
            }
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("New playlist...")
                onClicked: pageStack.push("NewPlaylistDialog.qml")
            }

            SearchPullDownMenuItem { }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No playlists")
        }
    }
}
