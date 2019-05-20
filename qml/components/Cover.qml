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

CoverBackground {
    CoverPlaceholder {
        id: placeholder

        icon.source: "image://theme/harbour-unplayer"
        text: "Unplayer"
        visible: Unplayer.Player.queue.currentIndex === -1
    }

    Item {
        anchors.fill: parent
        visible: !placeholder.visible

        Image {
            id: mediaArtImage
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            source: Unplayer.Player.queue.currentMediaArt
            sourceSize.height: parent.height
            asynchronous: true
        }

        OpacityRampEffect {
            direction: OpacityRamp.BottomToTop
            offset: 0.15
            slope: 1.0
            sourceItem: mediaArtImage
        }

        Column {
            anchors {
                left: parent.left
                leftMargin: Theme.paddingMedium
                right: parent.right
                rightMargin: Theme.paddingMedium
                top: parent.top
                topMargin: Theme.paddingMedium
            }
            spacing: Theme.paddingSmall

            Label {
                property int position: Unplayer.Player.position

                anchors.horizontalCenter: parent.horizontalCenter
                color: Theme.highlightColor
                font.pixelSize: position >= 3600000 ? Theme.fontSizeExtraLarge : Theme.fontSizeHuge
                opacity: Unplayer.Player.playing ? 1.0 : 0.4
                text: Format.formatDuration(position / 1000, position >= 3600000? Format.DurationLong :
                                                                                  Format.DurationShort)
            }

            Label {
                horizontalAlignment: implicitWidth > width ? Text.AlignLeft : Text.AlignHCenter
                color: Theme.highlightColor
                text: Unplayer.Player.queue.currentArtist
                truncationMode: TruncationMode.Fade
                width: parent.width
            }

            Label {
                horizontalAlignment: implicitWidth > width ? Text.AlignLeft : Text.AlignHCenter
                maximumLineCount: 3
                text: Unplayer.Player.queue.currentTitle
                truncationMode: TruncationMode.Elide
                width: parent.width
                wrapMode: Text.WordWrap
            }
        }
    }

    CoverActionList {
        enabled: !placeholder.visible
        iconBackground: true

        CoverAction {
            iconSource: "image://theme/icon-cover-previous-song"
            onTriggered: Unplayer.Player.queue.previous()
        }

        CoverAction {
            iconSource: Unplayer.Player.playing ? "image://theme/icon-cover-pause" :
                                         "image://theme/icon-cover-play"
            onTriggered: {
                if (Unplayer.Player.playing) {
                    Unplayer.Player.pause()
                } else {
                    Unplayer.Player.play()
                }
            }
        }

        CoverAction {
            iconSource: "image://theme/icon-cover-next-song"
            onTriggered: Unplayer.Player.queue.next()
        }
    }
}
