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
                visible: Unplayer.Player.queue.currentIsLocalFile
                text: qsTranslate("unplayer", "Edit tags")
                onClicked: pageStack.push("TagEditDialog.qml", { files: [Unplayer.Player.queue.currentFilePath] })
            }

            MenuItem {
                text: qsTranslate("unplayer", "Add to playlist")
                onClicked: pageStack.push("AddToPlaylistPage.qml", { tracks: Unplayer.Player.queue.getTrack(Unplayer.Player.queue.currentIndex) })
            }

            MenuItem {
                visible: Unplayer.Player.queue.currentIsLocalFile
                text: qsTranslate("unplayer", "Track information")
                onClicked: pageStack.push("TrackInfoPage.qml", { filePath: Unplayer.Player.queue.currentFilePath })
            }
        }

        PushUpMenu {
            MenuItem {
                text: Unplayer.Player.stopAfterEos ? qsTranslate("unplayer", "Stop after playing track: <font color=\"%1\">yes</font>").arg(Theme.highlightColor)
                                                   : qsTranslate("unplayer", "Stop after playing track: <font color=\"%1\">no</font>").arg(Theme.highlightColor)
                onClicked: Unplayer.Player.stopAfterEos = !Unplayer.Player.stopAfterEos
            }

            MenuItem {
                text: qsTranslate("unplayer", "Clear Queue")
                onClicked: {
                    Unplayer.Player.queue.clear()
                    pageStack.pop()
                }
            }
        }

        BackgroundItem {
            id: pressItem

            anchors {
                left: parent.left
                top: parent.top
            }

            onClicked: {
                queuePageLoader.active = true
                var page = queuePageLoader.item
                pageStack.push(page)
                page.positionViewAtCurrentIndex()
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

            Loader {
                id: queuePageLoader
                active: false
                sourceComponent: Component { QueuePage {} }
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

                enabled: Unplayer.Player.seekable

                handleVisible: false
                label: {
                    if (duration > 0) {
                        return "-%1 / %2".arg(Format.formatDuration(remaining / 1000, remaining >= 3600000 ? Format.DurationLong :
                                                                                                             Format.DurationShort))
                                .arg(Format.formatDuration(duration / 1000, duration >= 3600000 ? Format.DurationLong :
                                                                                                  Format.DurationShort))
                    }
                    return String()
                }
                minimumValue: 0
                maximumValue: duration > 0 ? duration : 1

                valueText: Format.formatDuration(value / 1000, value >= 3600000 ? Format.DurationLong :
                                                                                  Format.DurationShort)
                width: parent.width

                onPositionChanged: {
                    if (!pressed) {
                        value = position
                    }
                }
                onReleased: {
                    if (Unplayer.Player.seekable) {
                        Unplayer.Player.position = value
                    }
                }
            }


            Item {
                id: controlsParent
                width: parent.width
                height: controls.height + (codecInfoLabelLoader.active ? Theme.paddingLarge : 0)

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

                Loader {
                    id: codecInfoLabelLoader
                    anchors {
                        bottom: parent.bottom
                        horizontalCenter: parent.horizontalCenter
                    }
                    active: Unplayer.Settings.showNowPlayingCodecInfo
                    sourceComponent: Component {
                        Label {
                            visible: audioCodecInfo.sampleRate !== 0

                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryColor
                            text: {
                                var str
                                if (audioCodecInfo.bitDepth !== 0) {
                                    str = qsTranslate("unplayer", "%L1-bit \u00b7 %2 kHz \u00b7 %L3 kbit/s \u00b7 %4")
                                        .arg(audioCodecInfo.bitDepth)
                                } else if (audioCodecInfo.sampleRate !== 0) {
                                    str = qsTranslate("unplayer", "%1 kHz \u00b7 %L2 kbit/s \u00b7 %3")
                                } else {
                                    return ""
                                }
                                return str.arg((audioCodecInfo.sampleRate / 1000.0).toLocaleString(Qt.locale(), 'f', 1))
                                    .arg(audioCodecInfo.bitrate)
                                    .arg(audioCodecInfo.audioCodec)
                            }

                            Unplayer.TrackAudioCodecInfo {
                                id: audioCodecInfo
                                filePath: Unplayer.Player.queue.currentFilePath
                            }
                        }
                    }
                }
            }
        }
    }
}
