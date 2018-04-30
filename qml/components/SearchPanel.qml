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

DockedPanel {
    id: searchPanel

    property string searchText: searchField.text.trim()

    function focusSearchField() {
        searchField.forceActiveFocus()
    }

    function unfocusSearchField() {
        searchField.focus = false
    }

    width: parent.width
    height: Theme.itemSizeMedium

    opacity: open ? 1 : 0
    dock: Dock.Top

    onOpenChanged: {
        if (open)
            focusSearchField()
        else
            searchField.text = String()
    }

    Behavior on opacity {
        FadeAnimation { }
    }

    Item {
        anchors.horizontalCenter: parent.horizontalCenter
        implicitWidth: Theme.itemSizeHuge * 3 - Theme.horizontalPageMargin
        width: implicitWidth > parent.width ? parent.width : implicitWidth
        height: Math.max(searchField.height, closeButton.height)

        SearchField {
            id: searchField

            anchors {
                left: parent.left
                right: closeButton.left
                verticalCenter: parent.verticalCenter
            }
            enabled: open

            onTextChanged: listView.model.filterRegExp = new RegExp(Unplayer.Utils.escapeRegExp(text.trim()), "i")
        }

        IconButton {
            id: closeButton

            anchors {
                right: parent.right
                rightMargin: Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }
            icon.source: "image://theme/icon-m-close"

            onClicked: open = false
        }
    }
}
