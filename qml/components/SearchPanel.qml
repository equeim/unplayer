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

    Row {
        anchors.centerIn: parent

        SearchField {
            id: searchField

            property int filledWidth: searchPanel.width - closeButton.width

            anchors.verticalCenter: parent.verticalCenter
            implicitWidth: Theme.buttonWidthMedium * 2
            width: implicitWidth > filledWidth ? filledWidth : implicitWidth

            enabled: open

            onTextChanged: {
                var regExpText = Unplayer.Utils.escapeRegExp(text.trim())
                if (regExpText.toLowerCase() === regExpText)
                    listView.model.filterRegExp = new RegExp(regExpText, "i")
                else
                    listView.model.filterRegExp = new RegExp(regExpText)
            }
        }

        IconButton {
            id: closeButton
            anchors.verticalCenter: parent.verticalCenter
            icon.source: "image://theme/icon-m-close"
            onClicked: open = false
        }
    }
}
