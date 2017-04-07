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

import harbour.unplayer 0.1 as Unplayer

Page {
    id: albumPage

    property string artist
    property string displayedArtist
    property string album
    property string displayedAlbum
    property int tracksCount
    property int duration
    property string mediaArt

    SearchPanel {
        id: searchPanel
    }

    SelectionPanel {
        id: selectionPanel
        selectionText: qsTranslate("unplayer", "%n track(s) selected", String(), tracksProxyModel.selectedIndexesCount)

        PushUpMenu {
            MenuItem {
                enabled: tracksProxyModel.hasSelection
                text: qsTranslate("unplayer", "Add to queue")

                onClicked: {
                    Unplayer.Player.queue.addTracks(tracksModel.getTracks(tracksProxyModel.selectedSourceIndexes))
                    selectionPanel.showPanel = false
                }
            }

            MenuItem {
                enabled: tracksProxyModel.hasSelection
                text: qsTranslate("unplayer", "Add to playlist")
                onClicked: pageStack.push(addToPlaylistPage)

                Component {
                    id: addToPlaylistPage

                    AddToPlaylistPage {
                        tracks: tracksModel.getTracks(tracksProxyModel.selectedSourceIndexes)
                        Component.onDestruction: {
                            if (added) {
                                selectionPanel.showPanel = false
                            }
                        }
                    }
                }
            }
        }
    }

    SilicaListView {
        id: listView

        anchors {
            fill: parent
            bottomMargin: selectionPanel.visible ? selectionPanel.visibleSize : 0
            topMargin: searchPanel.visibleSize
        }
        clip: true

        header: AlbumPageHeader { }
        delegate: LibraryTrackDelegate { }
        model: Unplayer.FilterProxyModel {
            id: tracksProxyModel

            sourceModel: Unplayer.TracksModel {
                id: tracksModel

                allArtists: false
                allAlbums: false

                artist: albumPage.artist
                album: albumPage.album
            }
        }

        PullDownMenu {
            MenuItem {
                visible: !unknownArtist && !unknownAlbum
                text: qsTranslate("unplayer", "Set cover image")
                onClicked: pageStack.push(filePickerDialogComponent)

                Component {
                    id: filePickerDialogComponent
                    FilePickerDialog {
                        title: qsTranslate("unplayer", "Select Image")
                        fileIcon: "image://theme/icon-m-image"
                        nameFilters: Unplayer.Utils.imageNameFilters
                        onAccepted: Unplayer.LibraryUtils.setMediaArt(model.artist, model.album, filePath)
                    }
                }
            }

            SelectionMenuItem {
                text: qsTranslate("unplayer", "Select tracks")
            }

            SearchMenuItem { }
        }

        ListViewPlaceholder {
            text: qsTranslate("unplayer", "No tracks")
        }

        VerticalScrollDecorator { }
    }
}
