import QtQuick 2.2
import Sailfish.Silica 1.0

import harbour.unplayer 0.1 as Unplayer

import "../components"

Page {
    property NowPlayingPage nowPlayingPage

    function goToCurrent() {
        var visibleY = listView.visibleArea.yPosition * listView.contentHeight - listView.headerItem.height
        var visibleHeight = listView.visibleArea.heightRatio * listView.contentHeight
        var currentItemY = listView.currentItem.y

        if ((currentItemY + listView.currentItem.height) < visibleY ||
                currentItemY > (visibleY + visibleHeight))
            listView.positionViewAtIndex(listView.currentIndex, ListView.Center)
    }

    objectName: "queuePage"

    Connections {
        target: player.queue
        onCurrentIndexChanged: {
            if (player.queue.currentIndex === -1)
                pageStack.pop(pageStack.previousPage(nowPlayingPage))
        }
    }

    SearchListView {
        id: listView

        anchors.fill: parent
        currentIndex: player.queue.currentIndex
        headerTitle: qsTr("Queue")
        highlightFollowsCurrentItem: !player.queue.shuffle
        delegate: BaseTrackDelegate {
            id: trackDelegate

            current: model.index === queueProxyModel.proxyIndex(player.queue.currentIndex)
            menu: ContextMenu {
                MenuItem {
                    function remove() {
                        player.queue.remove(queueProxyModel.sourceIndex(model.index))
                        trackDelegate.menuOpenChanged.disconnect(remove)
                    }

                    text: qsTr("Remove")
                    onClicked: trackDelegate.menuOpenChanged.connect(remove)
                }
            }
            onClicked: {
                if (current) {
                    if (!player.playing)
                        player.play()
                } else {
                    player.queue.currentIndex = queueProxyModel.sourceIndex(model.index)
                    player.queue.currentTrackChanged()
                    if (player.queue.shuffle)
                        player.queue.resetNotPlayedTracks()
                }
            }
            ListView.onRemove: animateRemoval()
        }
        model: Unplayer.FilterProxyModel {
            id: queueProxyModel

            filterRoleName: "title"
            sourceModel: Unplayer.QueueModel {
                queue: player.queue
            }
        }

        Component.onCompleted: positionViewAtIndex(currentIndex, ListView.Center)

        PullDownMenu {
            id: pullDownMenu

            MenuItem {
                function clear() {
                    player.queue.clear()
                    pageStack.busyChanged.disconnect(clear)
                }

                text: qsTr("Clear")
                onClicked: {
                    player.queue.currentIndex = -1
                    player.queue.currentTrackChanged()
                    pageStack.busyChanged.connect(clear)
                }
            }

            MenuItem {
                text: qsTr("Search")
                onClicked: listView.showSearchField = true
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No tracks")
        }
    }
}
