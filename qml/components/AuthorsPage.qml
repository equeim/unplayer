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

Page {
    SilicaListView {
        anchors.fill: parent

        header: PageHeader {
            title: qsTranslate("unplayer", "Authors")
        }
        delegate: Column {
            anchors {
                left: parent.left
                leftMargin: Theme.horizontalPageMargin
                right: parent.right
                rightMargin: Theme.horizontalPageMargin
            }

            Label {
                text: ("<style type=\"text/css\">A { color: %1; }</style>" +
                       "%2 &lt;<a href=\"mailto:%3\">%3</a>&gt;")
                .arg(Theme.highlightColor)
                .arg(model.name)
                .arg(model.email)

                textFormat: Text.RichText
                onLinkActivated: Qt.openUrlExternally(link)

                Component.onCompleted: console.log(text)
            }

            Label {
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: model.type === "maintainer" ? qsTranslate("unplayer", "Maintainer") : String()
            }
        }

        model: ListModel {
            ListElement {
                name: "Alexey Rochev"
                email: "equeim@gmail.com"
                type: "maintainer"
            }
        }
    }
}
