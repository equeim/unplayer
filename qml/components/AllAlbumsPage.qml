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

Page {
    Binding {
        target: modalDialog
        property: "active"
        value: albumsModel.removingFiles
    }

    Binding {
        target: modalDialog
        property: "text"
        value: qsTranslate("unplayer", "Removing albums...")
    }

    SearchPanel {
        id: searchPanel
    }

    SelectionPanel {
        id: selectionPanel
        selectionText: qsTranslate("unplayer", "%n album(s) selected", String(), albumsProxyModel.selectedIndexesCount)

        PushUpMenu {
            MenuItem {
                enabled: albumsProxyModel.hasSelection
                text: qsTranslate("unplayer", "Add to queue")
                onClicked: {
                    Unplayer.Player.queue.addTracks(albumsModel.getTracksForAlbums(albumsProxyModel.selectedSourceIndexes))
                    selectionPanel.showPanel = false
                }
            }

            MenuItem {
                enabled: albumsProxyModel.hasSelection
                text: qsTranslate("unplayer", "Add to playlist")
                onClicked: pageStack.push(addToPlaylistPage)

                Component {
                    id: addToPlaylistPage

                    AddToPlaylistPage {
                        tracks: albumsModel.getTracksForAlbums(albumsProxyModel.selectedSourceIndexes)
                        Component.onDestruction: {
                            if (added) {
                                selectionPanel.showPanel = false
                            }
                        }
                    }
                }
            }

            MenuItem {
                enabled: albumsProxyModel.hasSelection
                text: qsTranslate("unplayer", "Remove")
                onClicked: pageStack.push(removeAlbumsDialog)

                Component {
                    id: removeAlbumsDialog

                    RemoveFilesDialog {
                        title: qsTranslate("unplayer", "Are you sure you want to remove %n selected albums?", String(), albumsProxyModel.selectedIndexesCount)
                        onAccepted: {
                            albumsModel.removeAlbums(albumsProxyModel.selectedSourceIndexes, deleteFiles)
                            selectionPanel.showPanel = false
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

        header: PageHeader {
            title: qsTranslate("unplayer", "Albums")
        }
        delegate: AlbumDelegate {
            description: model.displayedArtist
        }
        model: Unplayer.FilterProxyModel {
            id: albumsProxyModel

            filterRole: Unplayer.AlbumsModel.AlbumRole
            sourceModel: Unplayer.AlbumsModel {
                id: albumsModel
                allArtists: true
            }
        }
        section {
            property: {
                switch (albumsModel.sortMode) {
                case Unplayer.AlbumsModel.SortArtistAlbum:
                case Unplayer.AlbumsModel.SortArtistYear:
                    return "displayedArtist"
                default:
                    return String()
                }
            }
            delegate: SectionHeader {
                text: section
            }
        }

        PullDownMenu {
            MenuItem {
                text: qsTranslate("unplayer", "Sort")
                onClicked: pageStack.push("AlbumsSortPage.qml", {albumsModel: albumsModel})
            }

            SelectionMenuItem {
                text: qsTranslate("unplayer", "Select albums")
            }

            SearchMenuItem { }
        }

        ListViewPlaceholder {
            text: qsTranslate("unplayer", "No albums")
        }

        VerticalScrollDecorator { }
    }
}


