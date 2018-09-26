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

import harbour.unplayer 0.1 as Unplayer

Page {
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
                text: qsTranslate("unplayer", "Clear Queue")
                onClicked: {
                    Unplayer.Player.queue.clear()
                    pageStack.pop()
                }
            }

            MenuItem {
                text: qsTranslate("unplayer", "Add to playlist")
                onClicked: pageStack.push("AddToPlaylistPage.qml", { tracks: Unplayer.Player.queue.getTrack(Unplayer.Player.queue.currentIndex) })
            }

            MenuItem {
                text: qsTranslate("unplayer", "Track information")
                onClicked: pageStack.push("TrackInfoPage.qml", { filePath: Unplayer.Player.queue.currentFilePath })
            }
        }

        BackgroundItem {
            id: pressItem

            anchors {
                left: parent.left
                top: parent.top
            }

            onClicked: {
                var page = pageStack.push(queuePageComponent)
                if (page) {
                    page.positionViewAtCurrentIndex()
                }
            }

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
                    source: pressItem.highlighted ? "image://theme/icon-l-music?" + Theme.highlightColor :
                                                    "image://theme/icon-l-music"
                }
            }

            Image {
                id: mediaArtImage
                anchors.fill: parent
                asynchronous: true
                fillMode: Image.PreserveAspectCrop
                visible: status === Image.Ready
                source: Unplayer.Player.queue.currentMediaArt

                Rectangle {
                    anchors.fill: parent
                    color: Theme.highlightBackgroundColor
                    opacity: Theme.highlightBackgroundOpacity
                    visible: pressItem.highlighted
                }
            }

            Component {
                id: queuePageComponent
                QueuePage { }
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
                    text: Unplayer.Player.queue.currentTitle
                    truncationMode: TruncationMode.Fade
                    width: parent.width
                }

                Label {
                    font.pixelSize: Theme.fontSizeSmall
                    text: Unplayer.Player.queue.currentArtist
                    truncationMode: TruncationMode.Fade
                    width: parent.width
                }

                Label {
                    font.pixelSize: Theme.fontSizeSmall
                    text: Unplayer.Player.queue.currentAlbum
                    truncationMode: TruncationMode.Fade
                    width: parent.width
                }
            }

            Slider {
                id: progressBar

                property int duration: Unplayer.Player.duration
                property int remaining: duration - position
                property int position: Unplayer.Player.position

                handleVisible: false
                label: "-%1 / %2".arg(Format.formatDuration(remaining / 1000, remaining >= 3600000 ? Format.DurationLong :
                                                                                                      Format.DurationShort))
                                .arg(Format.formatDuration(duration / 1000, duration >= 3600000 ? Format.DurationLong :
                                                                                                  Format.DurationShort))
                minimumValue: 0
                maximumValue: duration ? duration : 1

                valueText: Format.formatDuration(value / 1000, value >= 3600000 ? Format.DurationLong :
                                                                                  Format.DurationShort)
                width: parent.width

                onPositionChanged: {
                    if (!pressed) {
                        value = position
                    }
                }
                onReleased: Unplayer.Player.position = value
            }

            Row {
                id: controls
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: largeScreen ? Theme.paddingLarge : Theme.paddingMedium

                Switch {
                    anchors.verticalCenter: parent.verticalCenter
                    checked: Unplayer.Player.queue.shuffle
                    icon.source: "image://theme/icon-m-shuffle"
                    onCheckedChanged: Unplayer.Player.queue.shuffle = checked
                }

                IconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    icon.source: "image://theme/icon-m-previous"
                    onClicked: Unplayer.Player.queue.previous()
                }

                IconButton {
                    id: playPauseButton

                    property bool playing: Unplayer.Player.playing

                    anchors.verticalCenter: parent.verticalCenter
                    icon.source: playing ? "image://theme/icon-l-pause" :
                                           "image://theme/icon-l-play"

                    onClicked: {
                        if (playing) {
                            Unplayer.Player.pause()
                        } else {
                            Unplayer.Player.play()
                        }
                    }
                }

                IconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    icon.source: "image://theme/icon-m-next"
                    onClicked: Unplayer.Player.queue.next()
                }

                Switch {
                    id: repeatSwitch

                    anchors.verticalCenter: parent.verticalCenter
                    icon.source: "image://theme/icon-m-repeat"
                    checked: Unplayer.Player.queue.repeatMode !== Unplayer.Queue.NoRepeat
                    automaticCheck: false

                    onClicked: Unplayer.Player.queue.changeRepeatMode()

                    Label {
                        anchors {
                            horizontalCenter: parent.horizontalCenter
                            bottom: parent.bottom
                            bottomMargin: Theme.paddingMedium
                        }
                        visible: Unplayer.Player.queue.repeatMode === Unplayer.Queue.RepeatOne
                        color: repeatSwitch.pressed ? Theme.highlightColor : Theme.primaryColor
                        font.pixelSize: Theme.fontSizeExtraSmall
                        text: "1"
                    }
                }
            }
        }
    }
}
