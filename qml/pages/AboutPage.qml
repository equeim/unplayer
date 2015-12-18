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

Page {
    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTr("About")
            }

            Item {
                width: parent.width
                height: childrenRect.height

                Image {
                    id: icon

                    anchors.horizontalCenter: parent.horizontalCenter
                    asynchronous: true
                    source: {
                        var iconSize = Theme.iconSizeExtraLarge
                        if (iconSize < 108)
                            iconSize = 86
                        else if (iconSize < 128)
                            iconSize = 108
                        else if (iconSize < 256)
                            iconSize = 128
                        else iconSize = 256

                        return "/usr/share/icons/hicolor/" + iconSize + "x" + iconSize + "/apps/harbour-unplayer.png"
                    }
                }

                Column {
                    id: appTitleColumn

                    anchors {
                        left: parent.left
                        leftMargin: Theme.horizontalPageMargin
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        top: icon.bottom
                        topMargin: Theme.paddingMedium
                    }

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        font.pixelSize: Theme.fontSizeLarge
                        text: "Unplayer 0.3"
                    }

                    Label {
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("Simple music player for Sailfish OS")
                        width: parent.width
                        wrapMode: Text.WordWrap
                    }

                    Label {
                        horizontalAlignment: implicitWidth > width ? Text.AlignLeft : Text.AlignHCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                        font.pixelSize: Theme.fontSizeExtraSmall
                        text: "Copyright (C) 2015 Alexey Rochev <equeim@gmail.com>"
                        truncationMode: TruncationMode.Fade
                        width: parent.width
                    }
                }

                Column {
                    anchors {
                        top: appTitleColumn.bottom
                        topMargin: Theme.paddingLarge
                    }
                    width: parent.width
                    spacing: Theme.paddingMedium

                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Show license")
                        onClicked: pageStack.push("LicensePage.qml")
                    }

                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "GitHub"
                        onClicked: Qt.openUrlExternally("https://github.com/equeim/unplayer")
                    }

                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Translations")
                        onClicked: Qt.openUrlExternally("https://hosted.weblate.org/projects/unplayer/translations")
                    }
                }
            }

            SectionHeader {
                text: qsTr("Translators")
            }

            Label {
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                }
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.WordWrap
                text: "Nederlands
    Nathan Follens <nathan@email.is>

English
    Alexey Rochev <equeim@gmail.com>
    Nathan Follens <nathan@email.is>

Français
    Nathan Follens <nathan@email.is>

Deutsch
    velox <jngibbon@gmail.com>
    Nathan Follens <nathan@email.is>

Norsk bokmål
    Nathan Follens <nathan@email.is>

Русский
    Alexey Rochev <equeim@gmail.com>

Español
    Nathan Follens <nathan@email.is>

Svenska
    Åke Engelbrektson <eson57@users.noreply.github.com>
"
            }
        }

        VerticalScrollDecorator { }
    }
}
