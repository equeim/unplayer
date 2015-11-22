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

    property alias album: tracksModel.album
    property alias unknownAlbum: tracksModel.unknownAlbum

    property alias artist: tracksModel.artist
    property alias unknownArtist: tracksModel.unknownArtist

    property int tracksCount
    property int duration

    SearchListView {
        id: listView

        anchors.fill: parent
        header: AlbumPageHeader { }
        delegate: LibraryTrackDelegate {
            allAlbums: false
        }
        model: TracksProxyModel {
            id: tracksProxyModel

            sourceModel: TracksModel {
                id: tracksModel

                allArtists: false
                allAlbums: false
            }
        }

        LibraryTracksPullDownMenu { }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No tracks")
        }
    }
}
