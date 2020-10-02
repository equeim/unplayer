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

DockedPanel {
    id: panel

    property bool shouldBeClosed
    property int itemHeight: largeScreen ? Theme.itemSizeExtraLarge : Theme.itemSizeMedium

    width: parent.width
    height: column.height

    Behavior on height {
        PropertyAnimation { }
    }

    Binding {
        target: panel
        property: "open"
        value: (firstItem.active || libraryScanItemLoader.itemActive) &&
               !Qt.inputMethod.visible &&
               !!pageStack.currentPage &&
               !shouldBeClosed
    }

    NowPlayingPage {
        id: nowPlayingPage
    }

    Column {
        id: column
        width: parent.width

        Item {
            id: firstItem

            property bool active: nowPlayingItem.active || busyItem.active

            width: parent.width
            height: itemHeight
            visible: nowPlayingItem.shouldBeVisible || busyItem.shouldBeVisible

            BackgroundItem {
                id: nowPlayingItem

                property bool active: Unplayer.Player.queue.currentIndex !== -1 &&
                                      pageStack.currentPage !== nowPlayingPage &&
                                      !busyItem.active

                property bool shouldBeVisible: active || opacity

                anchors.fill: parent

                enabled: active
                opacity: active ? 1.0 : 0.0
                visible: shouldBeVisible

                Behavior on opacity {
                    FadeAnimator { }
                }

                onClicked: {
                    if (pageStack.currentPage.objectName === "queuePage") {
                        pageStack.currentPage.goToCurrent()
                    } else {
                        if (!pageStack.find(function(page) {
                            if (page === nowPlayingPage) {
                                return true
                            }
                            return false
                        })) {
                            pageStack.push(nowPlayingPage)
                        }
                    }
                }

                Item {
                    id: progressBarItem

                    height: Theme.paddingSmall
                    width: parent.width

                    Rectangle {
                        id: progressBar

                        property int duration: Unplayer.Player.duration

                        height: parent.height
                        width: duration ? parent.width * (Unplayer.Player.position / duration) : 0
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
                    anchors.top: progressBarItem.bottom
                    width: parent.width
                    height: nowPlayingItem.height - progressBarItem.height

                    MediaArt {
                        id: mediaArt
                        highlighted: nowPlayingItem.highlighted
                        size: parent.height
                        source: Unplayer.Player.queue.currentMediaArt
                    }

                    Column {
                        anchors {
                            left: mediaArt.right
                            leftMargin: largeScreen ? Theme.paddingLarge : Theme.paddingMedium
                            right: buttonsRow.left
                            verticalCenter: parent.verticalCenter
                        }

                        Label {
                            visible: text
                            color: Theme.highlightColor
                            font.pixelSize: largeScreen ? Theme.fontSizeMedium : Theme.fontSizeSmall
                            text: Unplayer.Player.queue.currentArtist
                            truncationMode: TruncationMode.Fade
                            width: parent.width
                        }

                        Label {
                            font.pixelSize: largeScreen ? Theme.fontSizeMedium : Theme.fontSizeSmall
                            text: Unplayer.Player.queue.currentTitle
                            truncationMode: TruncationMode.Fade
                            width: parent.width
                        }
                    }

                    Row {
                        id: buttonsRow

                        readonly property bool pageIsLandscape: pageStack.currentPage && pageStack.currentPage.isLandscape
                        readonly property bool centerButtons: pageIsLandscape && largeScreen

                        anchors {
                            right: centerButtons ? undefined : parent.right
                            rightMargin: (!centerButtons && pageIsLandscape) ? Theme.horizontalPageMargin : 0
                            horizontalCenter: centerButtons ? parent.horizontalCenter : undefined
                        }
                        height: parent.height
                        spacing: largeScreen ? Theme.paddingLarge : 0

                        IconButton {
                            id: icon
                            height: parent.height
                            icon.source: "image://theme/icon-m-previous"
                            onClicked: Unplayer.Player.queue.previous()
                        }

                        IconButton {
                            property bool playing: Unplayer.Player.playing

                            height: parent.height
                            icon.source: {
                                if (playing) {
                                    if (largeScreen) {
                                        return "image://theme/icon-l-pause"
                                    }
                                    return "image://theme/icon-m-pause"
                                }

                                if (largeScreen) {
                                    return "image://theme/icon-l-play"
                                }
                                return "image://theme/icon-m-play"
                            }

                            onClicked: {
                                if (playing) {
                                    Unplayer.Player.pause()
                                } else {
                                    Unplayer.Player.play()
                                }
                            }
                        }

                        IconButton {
                            height: parent.height
                            icon.source: "image://theme/icon-m-next"
                            onClicked: Unplayer.Player.queue.next()
                        }
                    }
                }
            }

            Item {
                id: busyItem

                property bool active: Unplayer.Player.queue.addingTracks || Unplayer.LibraryUtils.savingTags || Unplayer.LibraryUtils.removingFiles

                property bool shouldBeVisible: active || opacity

                anchors.fill: parent

                opacity: active ? 1.0 : 0.0
                visible: shouldBeVisible

                Behavior on opacity {
                    FadeAnimator { }
                }

                BusyIndicator {
                    id: busyIndicator

                    anchors {
                        left: parent.left
                        leftMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    running: busyItem.visible
                    size: BusyIndicatorSize.Medium
                }

                Label {
                    id: busyLabel

                    anchors {
                        left: busyIndicator.right
                        leftMargin: Theme.paddingLarge
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    color: Theme.highlightColor
                    truncationMode: TruncationMode.Fade
                    text: {
                        if (Unplayer.Player.queue.addingTracks) {
                            return qsTranslate("unplayer", "Adding tracks...")
                        }
                        if (Unplayer.LibraryUtils.savingTags) {
                            return qsTranslate("unplayer", "Saving tags...")
                        }
                        if (Unplayer.LibraryUtils.removingFiles) {
                            return qsTranslate("unplayer", "Removing files...")
                        }
                        return ""
                    }
                }
            }
        }

        Loader {
            id: libraryScanItemLoader

            property bool itemActive: Unplayer.LibraryUtils.updating

            width: parent.width
            height: item ? item.height : 0
            visible: status === Loader.Ready
            opacity: itemActive && visible ? 1.0 : 0.0

            Behavior on opacity {
                FadeAnimator { }
            }

            active: itemActive || opacity
            asynchronous: true

            sourceComponent: Component {
                BackgroundItem {
                    width: parent ? parent.width : 0
                    height: itemHeight

                    onClicked: progressPanelComponent.incubateObject(rootWindow.contentItem)

                    Rectangle {
                        anchors.fill: parent
                        color: Theme.rgba("black", 0.1)
                    }

                    BusyIndicator {
                        id: indicator

                        anchors {
                            left: parent.left
                            leftMargin: Theme.horizontalPageMargin
                            verticalCenter: parent.verticalCenter
                        }
                        running: true
                        size: BusyIndicatorSize.Medium
                    }

                    LibraryUpdateLabels {
                        anchors {
                            left: indicator.right
                            leftMargin: Theme.paddingLarge
                            verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }

            Component {
                id: progressPanelComponent

                DockedPanel {
                    id: progressPanel

                    width: parent.width
                    height: panelColumn.height + Theme.paddingLarge * 2
                    modal: true

                    onMovingChanged: if (!moving && !open) {
                                         destroy()
                                     }

                    Component.onCompleted: show()

                    Connections {
                        target: libraryScanItemLoader
                        onItemActiveChanged: if (!libraryScanItemLoader.itemActive) {
                                                 hide()
                                             }
                    }

                    TouchBlocker {
                        anchors.fill: parent
                        parent: rootWindow.contentItem
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: Theme.overlayBackgroundColor
                    }

                    MouseArea {
                        anchors.fill: parent
                    }

                    Column {
                        id: panelColumn
                        anchors {
                            top: parent.top
                            topMargin: Theme.paddingLarge
                        }
                        width: parent.width
                        spacing: Theme.paddingLarge

                        BusyIndicator {
                            anchors.horizontalCenter: parent.horizontalCenter
                            size: BusyIndicatorSize.Large
                            running: true
                        }

                        LibraryUpdateLabels {
                            center: true
                        }

                        Button {
                            anchors.horizontalCenter: parent.horizontalCenter
                            preferredWidth: Theme.buttonWidthLarge
                            text: qsTranslate("unplayer", "Cancel")
                            onClicked: Unplayer.LibraryUtils.cancelDatabaseUpdate()
                        }
                    }
                }
            }
        }
    }
}
