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

import "../components"

Page {
    id: page

    property bool landscapeLayout: isLandscape && !largeScreen

    states: [
        State {
            name: "PORTRAIT"
            when: !landscapeLayout

            AnchorChanges {
                target: pressItem
                anchors {
                    right: parent.right
                    bottom: column.top
                }
            }

            PropertyChanges {
                target: pressItem
                anchors.bottomMargin: Theme.paddingLarge
            }

            PropertyChanges {
                target: mediaArtImage
                sourceSize.width: parent.width
                sourceSize.height: undefined
            }

            AnchorChanges {
                target: column
                anchors {
                    bottom: parent.bottom
                    verticalCenter: undefined
                }
            }

            PropertyChanges {
                target: column
                anchors.bottomMargin: Theme.paddingLarge
                width: parent.width
            }
        },
        State {
            name: "LANDSCAPE"
            when: landscapeLayout

            AnchorChanges {
                target: pressItem
                anchors {
                    right: column.left
                    bottom: parent.bottom
                }
            }

            PropertyChanges {
                target: pressItem
                anchors.bottomMargin: undefined
            }

            PropertyChanges {
                target: mediaArtImage
                sourceSize.width: undefined
                sourceSize.height: parent.height
            }

            AnchorChanges {
                target: column
                anchors {
                    bottom: undefined
                    verticalCenter: parent.verticalCenter
                }
            }

            PropertyChanges {
                target: column
                anchors.bottomMargin: undefined
                width: controls.width + Theme.horizontalPageMargin * 2
            }
        }
    ]

    SilicaFlickable {
        anchors.fill: parent

        PullDownMenu {
            MenuItem {
                text: qsTr("Add to playlist")
                onClicked: pageStack.push("AddToPlaylistPage.qml", { tracks: player.queue.currentUrl })
            }
        }

        BackgroundItem {
            id: pressItem

            anchors {
                left: parent.left
                top: parent.top
            }

            onClicked: pageStack.push("QueuePage.qml").positionViewAtCurrentIndex()

            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop {
                        position: 0.0;
                        color: Theme.rgba(Theme.primaryColor, 0.2)
                    }
                    GradientStop {
                        position: 1.0;
                        color: Theme.rgba(Theme.primaryColor, 0.1)
                    }
                }
                visible: !mediaArtImage.visible

                Image {
                    anchors.centerIn: parent
                    asynchronous: true
                    source: "image://theme/icon-l-music"
                }
            }

            Image {
                id: mediaArtImage
                anchors.fill: parent
                asynchronous: true
                cache: false
                fillMode: Image.PreserveAspectCrop
                visible: status === Image.Ready

                Binding {
                    target: mediaArtImage
                    property: "source"
                    value: player.queue.currentMediaArt
                }

                Connections {
                    target: rootWindow
                    onMediaArtReloadNeeded: {
                        mediaArtImage.source = String()
                        mediaArtImage.source = player.queue.currentMediaArt
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    color: Theme.highlightBackgroundColor
                    opacity: Theme.highlightBackgroundOpacity
                    visible: pressItem.highlighted
                }
            }
        }

        Column {
            id: column
            anchors.right: parent.right

            Column {
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                }

                Label {
                    color: Theme.highlightColor
                    text: player.queue.currentTitle
                    truncationMode: TruncationMode.Fade
                    width: parent.width
                }

                Label {
                    font.pixelSize: Theme.fontSizeSmall
                    text: player.queue.currentArtist
                    truncationMode: TruncationMode.Fade
                    width: parent.width
                }

                Label {
                    font.pixelSize: Theme.fontSizeSmall
                    text: player.queue.currentAlbum
                    truncationMode: TruncationMode.Fade
                    width: parent.width
                }
            }

            Slider {
                id: progressBar

                property int duration: player.queue.currentDuration
                property int position: player.position

                enabled: duration !== 0
                handleVisible: false
                label: Format.formatDuration(duration, duration >= 3600 ? Format.DurationLong :
                                                                          Format.DurationShort)
                maximumValue: duration === 0 ? 1 : duration * 1000

                valueText: Format.formatDuration(value / 1000, value >= 3600000 ? Format.DurationLong :
                                                                                  Format.DurationShort)
                width: parent.width

                onPositionChanged: {
                    if (!pressed)
                        value = position
                }
                onReleased: player.position = value
            }

            Row {
                id: controls
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: largeScreen ? Theme.paddingLarge : Theme.paddingMedium

                Switch {
                    anchors.verticalCenter: parent.verticalCenter
                    checked: player.queue.shuffle
                    icon.source: "image://theme/icon-m-shuffle"
                    onCheckedChanged: player.queue.shuffle = checked
                }

                IconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    icon.source: "image://theme/icon-m-previous"
                    onClicked: player.queue.previous()
                }

                IconButton {
                    id: playPauseButton

                    property bool playing: player.playing

                    anchors.verticalCenter: parent.verticalCenter
                    icon.source: playing ? "image://theme/icon-l-pause" :
                                           "image://theme/icon-l-play"

                    onClicked: {
                        if (playing)
                            player.pause()
                        else
                            player.play()
                    }
                }

                IconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    icon.source: "image://theme/icon-m-next"
                    onClicked: player.queue.next()
                }

                Switch {
                    anchors.verticalCenter: parent.verticalCenter
                    checked: player.queue.repeat
                    icon.source: "image://theme/icon-m-repeat"
                    onCheckedChanged: player.queue.repeat = checked
                }
            }
        }
    }
}
