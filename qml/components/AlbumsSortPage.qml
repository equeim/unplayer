/*
 * Unplayer
 * Copyright (C) 2015-2019 Alexey Rochev <equeim@gmail.com>
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
    property var albumsModel

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTranslate("unplayer", "Sort")
            }

            ComboBox {
                label: qsTranslate("unplayer", "Order")
                menu: ContextMenu {
                    MenuItem {
                        text: qsTranslate("unplayer", "Ascending")
                        onClicked: albumsModel.sortDescending = false
                    }

                    MenuItem {
                        text: qsTranslate("unplayer", "Descending")
                        onClicked: albumsModel.sortDescending = true
                    }
                }

                Component.onCompleted: {
                    if (albumsModel.sortDescending) {
                        currentIndex = 1
                    }
                }
            }

            Separator {
                width: parent.width
                color: Theme.secondaryColor
            }

            SortModeListItem {
                current: albumsModel.sortMode === Unplayer.AlbumsModel.SortAlbum
                text: qsTranslate("unplayer", "Album title")
                onClicked: albumsModel.sortMode = Unplayer.AlbumsModel.SortAlbum
            }

            SortModeListItem {
                current: albumsModel.sortMode === Unplayer.AlbumsModel.SortYear
                text: qsTranslate("unplayer", "Album year")
                onClicked: albumsModel.sortMode = Unplayer.AlbumsModel.SortYear
            }

            SortModeListItem {
                visible: albumsModel.allArtists
                current: albumsModel.sortMode === Unplayer.AlbumsModel.SortArtistAlbum
                text: qsTranslate("unplayer", "Artist - Album title")
                onClicked: albumsModel.sortMode = Unplayer.AlbumsModel.SortArtistAlbum
            }

            SortModeListItem {
                visible: albumsModel.allArtists
                current: albumsModel.sortMode === Unplayer.AlbumsModel.SortArtistYear
                text: qsTranslate("unplayer", "Artist - Album year")
                onClicked: albumsModel.sortMode = Unplayer.AlbumsModel.SortArtistYear
            }
        }

        VerticalScrollDecorator { }
    }
}
