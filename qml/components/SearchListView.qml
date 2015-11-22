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

SilicaListView {
    id: listView

    property bool showSearchField: false
    property alias searchFieldText: searchField.text
    property alias searchFieldHeight: searchField.height

    property string headerTitle

    header: Item {
        property int searchFieldY: childrenRect.height

        width: listView.width
        height: showSearchField ? childrenRect.height + searchFieldHeight :
                                  childrenRect.height

        Behavior on height {
            PropertyAnimation { }
        }

        PageHeader {
            title: headerTitle
        }
    }

    onShowSearchFieldChanged: {
        if (showSearchField)
            searchField.forceActiveFocus()
    }

    SearchField {
        id: searchField

        parent: listView.parent

        width: parent.width
        y: -contentY - headerItem.height + headerItem.searchFieldY
        z: 1

        enabled: showSearchField ? true : false
        opacity: showSearchField ? 1 : 0

        onActiveFocusChanged: {
            if (!activeFocus && text.length === 0)
                showSearchField = false
        }

        onTextChanged: {
            var regExpText = Unplayer.Utils.escapeRegExp(text.trim())
            if (regExpText.toLowerCase() === regExpText)
                model.filterRegExp = new RegExp(regExpText, "i")
            else
                model.filterRegExp = new RegExp(regExpText)
        }

        EnterKey.iconSource: "image://theme/icon-m-enter-close"
        EnterKey.onClicked: focus = false

        Behavior on opacity {
            FadeAnimation { }
        }
    }

    VerticalScrollDecorator { }
}
