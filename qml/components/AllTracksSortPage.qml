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
    property var tracksModel

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
                        onClicked: tracksModel.sortDescending = false
                    }

                    MenuItem {
                        text: qsTranslate("unplayer", "Descending")
                        onClicked: tracksModel.sortDescending = true
                    }
                }

                Component.onCompleted: {
                    if (tracksModel.sortDescending) {
                        currentIndex = 1
                    }
                }
            }

            Separator {
                width: parent.width
                color: Theme.secondaryColor
            }

            SortModeListItem {
                current: tracksModel.sortMode === Unplayer.TracksModelSortMode.Title
                text: qsTranslate("unplayer", "Title")
                onClicked: tracksModel.sortMode = Unplayer.TracksModelSortMode.Title
            }

            SortModeListItem {
                current: tracksModel.sortMode === Unplayer.TracksModelSortMode.AddedDate
                text: qsTranslate("unplayer", "Added date")
                onClicked: tracksModel.sortMode = Unplayer.TracksModelSortMode.AddedDate
            }

            SortModeListItem {
                current: tracksModel.sortMode === Unplayer.TracksModelSortMode.ArtistAlbumTitle
                text: tracksModel.allArtists ? qsTranslate("unplayer", "Artist - Album title")
                                             : qsTranslate("unplayer", "Album title")
                onClicked: tracksModel.sortMode = Unplayer.TracksModelSortMode.ArtistAlbumTitle
            }

            SortModeListItem {
                current: tracksModel.sortMode === Unplayer.TracksModelSortMode.ArtistAlbumYear
                text: tracksModel.allArtists ? qsTranslate("unplayer", "Artist - Album year")
                                             : qsTranslate("unplayer", "Album year")
                onClicked: tracksModel.sortMode = Unplayer.TracksModelSortMode.ArtistAlbumYear
            }

            SectionHeader {
                text: qsTranslate("unplayer", "Inside Album")
            }

            SortModeListItem {
                enabled: {
                    switch (tracksModel.sortMode) {
                    case Unplayer.TracksModelSortMode.ArtistAlbumTitle:
                    case Unplayer.TracksModelSortMode.ArtistAlbumYear:
                        return true
                    default:
                        return false
                    }
                }
                current: tracksModel.insideAlbumSortMode === Unplayer.TracksModelInsideAlbumSortMode.Title
                text: qsTranslate("unplayer", "Title")
                onClicked: tracksModel.insideAlbumSortMode = Unplayer.TracksModelInsideAlbumSortMode.Title
            }

            SortModeListItem {
                enabled: {
                    switch (tracksModel.sortMode) {
                    case Unplayer.TracksModelSortMode.ArtistAlbumTitle:
                    case Unplayer.TracksModelSortMode.ArtistAlbumYear:
                        return true
                    default:
                        return false
                    }
                }
                current: tracksModel.insideAlbumSortMode === Unplayer.TracksModelInsideAlbumSortMode.TrackNumber
                text: qsTranslate("unplayer", "Track number")
                onClicked: tracksModel.insideAlbumSortMode = Unplayer.TracksModelInsideAlbumSortMode.TrackNumber
            }
        }

        VerticalScrollDecorator { }
    }
}
