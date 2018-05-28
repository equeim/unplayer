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
                current: tracksModel.insideAlbumSortMode === Unplayer.TracksModelInsideAlbumSortMode.Title
                text: qsTranslate("unplayer", "Title")
                onClicked: tracksModel.insideAlbumSortMode = Unplayer.TracksModelInsideAlbumSortMode.Title
            }

            SortModeListItem {
                current: tracksModel.insideAlbumSortMode === Unplayer.TracksModelInsideAlbumSortMode.DiscNumberTitle
                text: qsTranslate("unplayer", "Disc number - Title")
                onClicked: tracksModel.insideAlbumSortMode = Unplayer.TracksModelInsideAlbumSortMode.DiscNumberTitle
            }

            SortModeListItem {
                current: tracksModel.insideAlbumSortMode === Unplayer.TracksModelInsideAlbumSortMode.DiscNumberTrackNumber
                text: qsTranslate("unplayer", "Disc number - Track number")
                onClicked: tracksModel.insideAlbumSortMode = Unplayer.TracksModelInsideAlbumSortMode.DiscNumberTrackNumber
            }
        }

        VerticalScrollDecorator { }
    }
}
