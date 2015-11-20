import QtQuick 2.2
import Sailfish.Silica 1.0

import "../pages"

DockedPanel {
    id: panel

    width: parent.width
    height: largeScreen ? Theme.itemSizeExtraLarge : Theme.itemSizeMedium

    Binding {
        target: panel
        property: "open"
        value: pageStack.currentPage !== nowPlayingPage &&
               player.queue.currentIndex !== -1
    }

    NowPlayingPage {
        id: nowPlayingPage
    }

    BackgroundItem {
        id: pressItem

        anchors.fill: parent
        onClicked: {
            if (pageStack.currentPage.objectName === "queuePage")
                pageStack.currentPage.goToCurrent()
            else
                pageStack.push(nowPlayingPage)
        }

        Item {
            id: progressBarItem

            height: Theme.paddingSmall
            width: parent.width

            Rectangle {
                id: progressBar

                property int duration: player.queue.currentDuration

                height: parent.height
                width: duration === 0 ? 0 : parent.width * (player.position / (duration * 1000))
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
                source: player.queue.currentMediaArt
                size: parent.height
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
                        if (playing)
                            player.pause()
                        else
                            player.play()
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
