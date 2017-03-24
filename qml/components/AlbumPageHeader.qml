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

Item {
    property alias searchFieldY: pageHeader.height

    width: listView.width
    height: listView.showSearchField ? pageHeader.height + listView.searchFieldHeight :
                                       childrenRect.height

    Behavior on height {
        PropertyAnimation { }
    }

    PageHeader {
        id: pageHeader
        title: model.album
    }

    Rectangle {
        anchors {
            left: parent.left
            right: parent.right
            top: pageHeader.bottom
        }
        height: Theme.itemSizeExtraLarge + Theme.paddingLarge * 2

        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: "transparent"
            }
            GradientStop {
                position: 1.0
                color: Theme.rgba(Theme.highlightBackgroundColor, 0.2)
            }
        }

        opacity: listView.showSearchField ? 0 : 1

        Behavior on opacity {
            FadeAnimation { }
        }

        MediaArt {
            id: mediaArt

            anchors.verticalCenter: parent.verticalCenter
            x: Theme.horizontalPageMargin

            source: albumDelegate.mediaArt
            size: Theme.itemSizeExtraLarge
        }

        Column {
            anchors {
                left: mediaArt.right
                leftMargin: Theme.paddingLarge
                right: parent.right
                rightMargin: Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }

            Label {
                font.pixelSize: Theme.fontSizeSmall
                text: model.artist
                textFormat: Text.StyledText
                width: parent.width
                truncationMode: TruncationMode.Fade
            }

            Label {
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("%n track(s)", String(), model.tracksCount)
                width: parent.width
                truncationMode: TruncationMode.Fade
            }

            Label {
                font.pixelSize: Theme.fontSizeSmall
                text: Unplayer.Utils.formatDuration(model.duration)
                width: parent.width
                truncationMode: TruncationMode.Fade
            }
        }
    }
}
