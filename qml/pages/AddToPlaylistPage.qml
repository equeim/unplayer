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

import Sailfish.Silica 1.0

import harbour.unplayer 0.1 as Unplayer

import "../components"
import "../models"

Page {
    id: page

    property var tracks

    SearchListView {
        id: listView

        anchors.fill: parent
        headerTitle: qsTr("Add to playlist")
        delegate: MediaContainerListItem {
            title: Theme.highlightText(model.title, listView.searchFieldText.trim(), Theme.highlightColor)
            description: model.tracksCount === undefined ? String() :
                                                           qsTr("%n track(s)", String(), model.tracksCount)
            onClicked: {
                Unplayer.PlaylistUtils.addTracksToPlaylist(model.url, tracks)
                pageStack.pop()
            }
        }
        model: Unplayer.FilterProxyModel {
            filterRoleName: "title"
            sourceModel: PlaylistsModel { }
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("New playlist...")
                onClicked: pageStack.push("NewPlaylistDialog.qml", {
                                              acceptDestination: pageStack.previousPage(),
                                              acceptDestinationAction: PageStackAction.Pop,
                                              tracks: page.tracks
                                          })
            }

            MenuItem {
                enabled: listView.count !== 0
                text: qsTr("Search")
                onClicked: listView.showSearchField = true
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No playlists")
        }
    }
}
