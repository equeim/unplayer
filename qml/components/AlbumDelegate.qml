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

MediaContainerSelectionDelegate {
    id: albumDelegate

    title: Theme.highlightText(model.displayedAlbum, searchPanel.searchText, Theme.highlightColor)
    secondDescription: model.year
    mediaArt: Unplayer.LibraryUtils.randomMediaArtForAlbum(model.artist, model.album)

    menu: Component {
        ContextMenu {
            MenuItem {
                text: qsTranslate("unplayer", "Add to queue")
                onClicked: Unplayer.Player.queue.addTracksFromLibrary(albumsModel.getTracksForAlbum(albumsProxyModel.sourceIndex(model.index)))
            }

            MenuItem {
                text: qsTranslate("unplayer", "Add to playlist")
                onClicked: pageStack.push("AddToPlaylistPage.qml", { tracks: albumsModel.getTracksForAlbum(albumsProxyModel.sourceIndex(model.index)) })
            }

            MenuItem {
                visible: !model.unknownArtist && !model.unknownAlbum
                text: qsTranslate("unplayer", "Set cover image")
                onClicked: pageStack.push(filePickerDialogComponent)
            }

            MenuItem {
                text: qsTranslate("unplayer", "Remove")
                onClicked: pageStack.push(removeAlbumDialog)
            }
        }
    }

    onClicked: {
        if (selectionPanel.showPanel) {
            listView.model.select(model.index)
        } else {
            pageStack.push(albumPageComponent)
        }
    }

    Connections {
        target: Unplayer.LibraryUtils
        onMediaArtChanged: mediaArt = Unplayer.LibraryUtils.randomMediaArtForAlbum(model.artist, model.album)
    }

    Component {
        id: albumPageComponent
        AlbumPage {
            artist: model.artist
            displayedArtist: model.displayedArtist
            album: model.album
            displayedAlbum: model.displayedAlbum
            tracksCount: model.tracksCount
            duration: model.duration
            mediaArt: albumDelegate.mediaArt
        }
    }

    Component {
        id: filePickerDialogComponent
        FilePickerDialog {
            title: qsTranslate("unplayer", "Select Image")
            fileIcon: "image://theme/icon-m-image"
            nameFilters: Unplayer.Utils.imageNameFilters
            onAccepted: Unplayer.LibraryUtils.setMediaArt(model.artist, model.album, filePath)
        }
    }

    Component {
        id: removeAlbumDialog

        RemoveFilesDialog {
            title: qsTranslate("unplayer", "Are you sure you want to remove this album?")
            onAccepted: {
                albumsModel.removeAlbum(albumsProxyModel.sourceIndex(model.index), deleteFiles)
            }
        }
    }
}
