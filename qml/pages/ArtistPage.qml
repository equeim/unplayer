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

    SearchListView {
        id: listView

        anchors.fill: parent
        header: ArtistPageHeader { }
        delegate: AlbumDelegate {
            description: qsTr("%n track(s)", String(), model.tracksCount)
        }
        model: Unplayer.FilterProxyModel {
            filterRoleName: "album"
            sourceModel: AlbumsModel {
                allArtists: false
                unknownArtist: artistDelegate.unknownArtist
                artist: model.rawArtist
            }
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Add to playlist")
                onClicked: pageStack.push("AddToPlaylistPage.qml", { tracks: getTracks() })
            }

            MenuItem {
                text: qsTr("Add to queue")
                onClicked: addTracksToQueue()
            }

            MenuItem {
                text: qsTr("All tracks")
                onClicked: pageStack.push("AllTracksPage.qml", {
                                              title: model.artist,
                                              unknownArtist: artistDelegate.unknownArtist,
                                              artist: model.rawArtist
                                          })
            }

            MenuItem {
                text: qsTr("Search")
                onClicked: listView.showSearchField = true
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No albums")
        }
    }
}
