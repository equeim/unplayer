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

import harbour.unplayer 0.1

ListItem {
    id: listItem

    property alias title: titleLabel.text
    property alias description: descriptionLabel.text
    property string mediaArt: randomMediaArt ? randomMediaArt.mediaArt : ""
    property alias fallbackIcon: mediaArt.fallbackIcon

    property RandomMediaArt randomMediaArt

    contentHeight: Theme.itemSizeExtraLarge

    MediaArt {
        id: mediaArt

        enabled: listItem.enabled
        highlighted: listItem.highlighted
        size: contentHeight
        x: Theme.horizontalPageMargin

        source: listItem.mediaArt
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
            id: titleLabel

            width: parent.width
            opacity: listItem.enabled ? 1.0 : 0.4
            color: highlighted ? Theme.highlightColor : Theme.primaryColor
            font.pixelSize: Theme.fontSizeLarge
            textFormat: Text.StyledText
            truncationMode: TruncationMode.Fade
        }

        Label {
            id: descriptionLabel
            color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
            font.pixelSize: Theme.fontSizeSmall
            truncationMode: TruncationMode.Fade
            visible: text
            width: parent.width
        }
    }
}
