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

import harbour.unplayer 0.1 as Unplayer

import "models"

Page {
    property alias bottomPanelOpen: selectionPanel.open

    function positionViewAtCurrentIndex() {
        listView.positionViewAtIndex(listView.currentIndex, ListView.Center)
    }

    function goToCurrent() {
        var visibleY = listView.visibleArea.yPosition * listView.contentHeight - listView.headerItem.height
        var visibleHeight = listView.visibleArea.heightRatio * listView.contentHeight
        var currentItemY = listView.currentItem.y

        if ((currentItemY + listView.currentItem.height) < visibleY ||
                currentItemY > (visibleY + visibleHeight))
            positionViewAtCurrentIndex()
    }

    objectName: "queuePage"

    Connections {
        target: player.queue
        onCurrentTrackChanged: {
            if (player.queue.currentIndex === -1)
                pageStack.pop(pageStack.previousPage(pageStack.previousPage()))
        }
    }

    SearchPanel {
        id: searchPanel
    }

    SelectionPanel {
        id: selectionPanel
        selectionText: qsTr("%n track(s) selected", String(), queueProxyModel.selectedIndexesCount)

        PushUpMenu {
            AddToPlaylistMenuItem {
                enabled: queueProxyModel.selectedIndexesCount !== 0
                onClicked: pageStack.push(addToPlaylistPage)

                Component {
                    id: addToPlaylistPage

                    AddToPlaylistPage {
                        tracks: selectionPanel.getSelectedTracks()
                        Component.onDestruction: {
                            if (added)
                                selectionPanel.showPanel = false
                        }
                    }
                }
            }

            MenuItem {
                enabled: queueProxyModel.selectedIndexesCount !== 0
                text: qsTr("Remove")
                onClicked: {
                    player.queue.removeTracks(queueProxyModel.selectedSourceIndexes())
                    selectionPanel.showPanel = false
                }
            }
        }
    }

    SilicaListView {
        id: listView

        anchors {
            fill: parent
            bottomMargin: {
                if (selectionPanel.expanded)
                    return selectionPanel.visibleSize
                if (nowPlayingPanel.expanded)
                    return nowPlayingPanel.height - nowPlayingPanel.visibleSize
            }
            topMargin: searchPanel.visibleSize
        }
        clip: true

        currentIndex: player.queue.currentIndex
        highlightFollowsCurrentItem: !player.queue.shuffle

        header: PageHeader {
            title: qsTr("Queue")
        }
        delegate: BaseTrackDelegate {
            id: trackDelegate

            showArtistAndAlbum: true

            current: model.index === queueProxyModel.proxyIndex(player.queue.currentIndex)
            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Track information")
                    onClicked: pageStack.push("TrackInfoPage.qml", { trackUrl: model.url })
                }

                MenuItem {
                    text: qsTr("Add to playlist")
                    onClicked: pageStack.push("AddToPlaylistPage.qml", { tracks: model.url })
                }

                MenuItem {
                    function remove() {
                        player.queue.removeTrack(queueProxyModel.sourceIndex(model.index))
                        trackDelegate.menuOpenChanged.disconnect(remove)
                    }

                    text: qsTr("Remove")
                    onClicked: trackDelegate.menuOpenChanged.connect(remove)
                }
            }
            onClicked: {
                if (selectionPanel.showPanel) {
                    queueProxyModel.select(model.index)
                } else {
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
            }
            ListView.onRemove: animateRemoval()
        }
        model: TracksProxyModel {
            id: queueProxyModel

            filterRoleName: "title"
            sourceModel: Unplayer.QueueModel {
                queue: player.queue
            }
        }

        PullDownMenu {
            SelectionMenuItem {
                text: qsTr("Select tracks")
            }

            SearchMenuItem { }
        }

        ListViewPlaceholder {
            text: qsTr("No tracks")
        }

        VerticalScrollDecorator { }
    }
}
