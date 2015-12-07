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

import "../pages"

MediaContainerListItem {
    id: albumDelegate

    property bool unknownAlbum: model.rawAlbum === undefined
    property bool unknownArtist: model.rawArtist === undefined

    function getTracks() {
        return sparqlConnection.select(Unplayer.Utils.tracksSparqlQuery(false,
                                                                        false,
                                                                        model.artist,
                                                                        unknownArtist,
                                                                        model.album,
                                                                        unknownAlbum))
    }

    function reloadMediaArt() {
        mediaArt = String()
        mediaArt = Unplayer.Utils.mediaArt(model.rawArtist, model.rawAlbum)
        if (typeof artistDelegate !== "undefined")
            artistDelegate.reloadMediaArt()
        rootWindow.mediaArtReloadNeeded()
    }

    title: Theme.highlightText(model.album, listView.searchFieldText.trim(), Theme.highlightColor)
    mediaArt: Unplayer.Utils.mediaArt(model.rawArtist, model.rawAlbum)
    menu: ContextMenu {
        MenuItem {
            text: qsTr("Add to queue")
            onClicked: {
                player.queue.add(getTracks())
                player.queue.setCurrentToFirstIfNeeded()
            }
        }

        MenuItem {
            text: qsTr("Add to playlist")
            onClicked: pageStack.push("../pages/AddToPlaylistPage.qml", { tracks: getTracks() })
        }

        MenuItem {
            enabled: !unknownArtist && !unknownAlbum
            text: qsTr("Set cover image")
            onClicked: pageStack.push(setCoverPage)

            Component {
                id: setCoverPage
                SetCoverPage { }
            }
        }
    }

    onClicked: pageStack.push(albumPage)

    Component {
        id: albumPage
        AlbumPage { }
    }
}
