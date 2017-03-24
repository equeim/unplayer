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

import "../pages"

DockedPanel {
    id: panel

    width: parent.width
    height: largeScreen ? Theme.itemSizeExtraLarge : Theme.itemSizeMedium

    opacity: open ? 1 : 0

    Behavior on opacity {
        FadeAnimation { }
    }

    Binding {
        target: panel
        property: "open"
        value: player.queue.currentIndex !== -1 &&
               !Qt.inputMethod.visible &&
               pageStack.currentPage !== nowPlayingPage &&
               (typeof pageStack.currentPage.bottomPanelOpen === "boolean" ? !pageStack.currentPage.bottomPanelOpen :
                                                                             true)
    }

    NowPlayingPage {
        id: nowPlayingPage
    }

    BackgroundItem {
        id: pressItem

        anchors.fill: parent
        onClicked: {
            if (pageStack.currentPage.objectName === "queuePage") {
                pageStack.currentPage.goToCurrent()
            } else {
                if (!pageStack.find(function(page) {
                    if (page === nowPlayingPage)
                        return true
                    return false
                }))
                    pageStack.push(nowPlayingPage)
            }
        }

        Item {
            id: progressBarItem

            height: Theme.paddingSmall
            width: parent.width

            Rectangle {
                id: progressBar

                property int duration: player.duration

                height: parent.height
                width: duration === 0 ? 0 : parent.width * (player.position / duration)
                color: Theme.highlightColor
                opacity: 0.5
            }

            Rectangle {
                anchors {
                    left: progressBar.right
                    right: parent.right
                }
                height: parent.height
                color: "black"
                opacity: Theme.highlightBackgroundOpacity
            }
        }

        Item {
            anchors {
                top: progressBarItem.bottom
                bottom: parent.bottom
            }
            width: parent.width

            MediaArt {
                id: mediaArt
                highlighted: pressItem.highlighted
                size: parent.height

                Binding {
                    target: mediaArt
                    property: "source"
                    value: player.queue.currentMediaArt
                }

                Connections {
                    target: rootWindow
                    onMediaArtReloadNeeded: {
                        mediaArt.source = String()
                        mediaArt.source = player.queue.currentMediaArt
                    }
                }
            }

            Column {
                anchors {
                    left: mediaArt.right
                    leftMargin: largeScreen ? Theme.paddingLarge : Theme.paddingMedium
                    right: buttonsRow.left
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    color: Theme.highlightColor
                    font.pixelSize: largeScreen ? Theme.fontSizeMedium : Theme.fontSizeSmall
                    text: player.queue.currentArtist
                    truncationMode: TruncationMode.Fade
                    width: parent.width
                }

                Label {
                    font.pixelSize: largeScreen ? Theme.fontSizeMedium : Theme.fontSizeSmall
                    text: player.queue.currentTitle
                    truncationMode: TruncationMode.Fade
                    width: parent.width
                }
            }

            Row {
                id: buttonsRow
                anchors {
                    right: largeScreen ? undefined : parent.right
                    horizontalCenter: largeScreen ? parent.horizontalCenter : undefined
                }
                height: parent.height
                spacing: largeScreen ? Theme.paddingLarge : 0

                IconButton {
                    id: icon
                    height: parent.height
                    icon.source: "image://theme/icon-m-previous"
                    onClicked: player.queue.previous()
                }

                IconButton {
                    property bool playing: player.playing

                    height: parent.height
                    icon.source: {
                        if (playing) {
                            if (largeScreen)
                                return "image://theme/icon-l-pause"
                            return "image://theme/icon-m-pause"
                        }

                        if (largeScreen)
                            return "image://theme/icon-l-play"
                        return "image://theme/icon-m-play"
                    }

                    onClicked: {
                        if (playing) {
                            player.pause()
                        } else {
                            player.play()
                        }
                    }
                }

                IconButton {
                    height: parent.height
                    icon.source: "image://theme/icon-m-next"
                    onClicked: player.queue.next()
                }
            }
        }
    }
}
