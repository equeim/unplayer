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

Dialog {
    canAccept: !urlField.errorHighlight

    onAccepted: Unplayer.Player.queue.addTrackFromUrl(urlField.text, false, 0)

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            DialogHeader {
                title: qsTranslate("unplayer", "Open URL")
                acceptText: qsTranslate("unplayer", "Open")
            }

            TextField {
                id: urlField

                width: parent.width

                label: qsTranslate("unplayer", "URL")
                placeholderText: label
                inputMethodHints: Qt.ImhNoAutoUppercase
                errorHighlight: !text.trim()

                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: accept()

                Component.onCompleted: {
                    text = Clipboard.text
                    forceActiveFocus()
                }
            }
        }
    }
}

