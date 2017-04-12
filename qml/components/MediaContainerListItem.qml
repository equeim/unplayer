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

ListItem {
    id: listItem

    property alias title: titleLabel.text
    property alias description: descriptionLabel.text
    property alias secondDescription: secondDescriptionLabel.text
    property alias mediaArt: mediaArt.source

    contentHeight: Theme.itemSizeLarge

    MediaArt {
        id: mediaArt

        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
        }
        size: contentHeight
        highlighted: listItem.highlighted
    }

    Item {
        anchors {
            left: mediaArt.right
            leftMargin: Theme.paddingLarge
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        height: childrenRect.height

        Label {
            id: titleLabel

            width: parent.width
            color: highlighted ? Theme.highlightColor : Theme.primaryColor
            textFormat: Text.StyledText
            truncationMode: TruncationMode.Fade
        }

        Label {
            id: descriptionLabel

            anchors {
                left: parent.left
                right: secondDescriptionLabel.left
                top: titleLabel.bottom
            }
            color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
            font.pixelSize: Theme.fontSizeSmall
            truncationMode: TruncationMode.Fade
        }

        Label {
            id: secondDescriptionLabel

            anchors {
                right: parent.right
                bottom: descriptionLabel.bottom
            }
            color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
            font.pixelSize: Theme.fontSizeSmall
        }
    }
}
