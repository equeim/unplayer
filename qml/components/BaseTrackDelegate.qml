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
    id: trackDelegate

    property bool current

    property bool showArtistAndAlbum
    property bool showAlbum
    property bool showDuration

    showMenuOnPressAndHold: !selectionPanel.showPanel

    Binding {
        target: trackDelegate
        property: "highlighted"
        value: down || menuOpen || listView.model.isSelected(model.index)
    }

    Connections {
        target: listView.model
        onSelectionChanged: trackDelegate.highlighted = listView.model.isSelected(model.index)
    }

    Column {
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            right: durationLabel.left
            rightMargin: Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }

        Label {
            color: highlighted || current ? Theme.highlightColor : Theme.primaryColor
            text: Theme.highlightText(model.title, searchPanel.searchText, Theme.highlightColor)
            textFormat: Text.StyledText
            truncationMode: TruncationMode.Fade
            width: parent.width
        }

        Label {
            visible: showArtistAndAlbum || showAlbum
            color: highlighted || current ? Theme.secondaryHighlightColor : Theme.secondaryColor
            font.pixelSize: Theme.fontSizeExtraSmall
            text: {
                if (showArtistAndAlbum) {
                    return qsTr("%1 - %2").arg(model.artist).arg(model.album)
                }

                if (showAlbum) {
                    return model.album
                }

                return String()
            }

            textFormat: Text.StyledText
            truncationMode: TruncationMode.Fade
            width: parent.width
        }
    }

    Label {
        id: durationLabel

        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            bottom: parent.bottom
            bottomMargin: Theme.paddingSmall
        }

        color: highlighted || current ? Theme.secondaryHighlightColor : Theme.secondaryColor
        font.pixelSize: Theme.fontSizeExtraSmall
        text: {
            if (showDuration) {
                return Format.formatDuration(model.duration, model.duration >= 3600? Format.DurationLong :
                                                                                     Format.DurationShort)
            }
            return String()
        }
    }
}
