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
    id: queuePage

    function positionViewAtCurrentIndex() {
        listView.positionViewAtIndex(listView.currentIndex, ListView.Center)
    }

    function goToCurrent() {
        var visibleY = listView.visibleArea.yPosition * listView.contentHeight - listView.headerItem.height
        var visibleHeight = listView.visibleArea.heightRatio * listView.contentHeight
        var currentItemY = listView.currentItem.y

        if ((currentItemY + listView.currentItem.height) < visibleY ||
                currentItemY > (visibleY + visibleHeight)) {
            positionViewAtCurrentIndex()
        }
    }

    objectName: "queuePage"

    Connections {
        target: Unplayer.Player.queue
        onCurrentTrackChanged: {
            if (Unplayer.Player.queue.currentIndex === -1 && pageStack.currentPage === queuePage) {
                pageStack.pop(pageStack.previousPage(pageStack.previousPage()))
            }
        }
    }

    SearchPanel {
        id: searchPanel
    }

    SelectionPanel {
        id: selectionPanel
        selectionText: qsTranslate("unplayer", "%n track(s) selected", String(), queueProxyModel.selectedIndexesCount)

        PushUpMenu {
            MenuItem {
                enabled: queueProxyModel.hasSelection
                text: qsTranslate("unplayer", "Add to playlist")
                onClicked: pageStack.push(addToPlaylistPage)

                Component {
                    id: addToPlaylistPage

                    AddToPlaylistPage {
                        tracks: Unplayer.Player.queue.getTracks(queueProxyModel.selectedSourceIndexes)
                        Component.onDestruction: {
                            if (added) {
                                selectionPanel.showPanel = false
                            }
                        }
                    }
                }
            }

            MenuItem {
                visible: Unplayer.Player.queue.hasLocalFileForTracks(queueProxyModel.selectedSourceIndexes)
                enabled: queueProxyModel.hasSelection
                text: qsTranslate("unplayer", "Edit tags")
                onClicked: pageStack.push("TagEditDialog.qml", {files: Unplayer.Player.queue.getTrackPaths(queueProxyModel.selectedSourceIndexes)})
            }

            MenuItem {
                enabled: queueProxyModel.hasSelection
                text: qsTranslate("unplayer", "Remove")
                onClicked: {
                    Unplayer.Player.queue.removeTracks(queueProxyModel.selectedSourceIndexes)
                    selectionPanel.showPanel = false
                }
            }
        }
    }

    SilicaListView {
        id: listView

        anchors {
            fill: parent
            bottomMargin: selectionPanel.visible ? selectionPanel.visibleSize : 0
            topMargin: searchPanel.visibleSize
        }
        clip: true

        currentIndex: Unplayer.Player.queue.currentIndex
        highlightFollowsCurrentItem: !Unplayer.Player.queue.shuffle

        header: PageHeader {
            title: qsTranslate("unplayer", "Queue")
        }
        delegate: BaseTrackDelegate {
            id: trackDelegate

            showArtistAndAlbum: model.isLocalFile
            showUrl: !showArtistAndAlbum

            current: model.index === queueProxyModel.proxyIndex(Unplayer.Player.queue.currentIndex)
            menu: ContextMenu {
                MenuItem {
                    visible: model.isLocalFile
                    text: qsTranslate("unplayer", "Track information")
                    onClicked: pageStack.push("TrackInfoPage.qml", { filePath: model.filePath })
                }

                MenuItem {
                    text: qsTranslate("unplayer", "Add to playlist")
                    onClicked: pageStack.push("AddToPlaylistPage.qml", { tracks: Unplayer.Player.queue.getTrack(queueProxyModel.sourceIndex(model.index)) })
                }

                MenuItem {
                    visible: model.isLocalFile
                    text: qsTranslate("unplayer", "Edit tags")
                    onClicked: pageStack.push("TagEditDialog.qml", {files: [model.filePath]})
                }

                MenuItem {
                    function remove() {
                        Unplayer.Player.queue.removeTrack(queueProxyModel.sourceIndex(model.index))
                        trackDelegate.menuOpenChanged.disconnect(remove)
                    }

                    text: qsTranslate("unplayer", "Remove")
                    onClicked: trackDelegate.menuOpenChanged.connect(remove)
                }
            }
            onClicked: {
                if (selectionPanel.showPanel) {
                    queueProxyModel.select(model.index)
                } else {
                    if (current) {
                        if (!Unplayer.Player.playing) {
                            Unplayer.Player.play()
                        }
                    } else {
                        Unplayer.Player.queue.currentIndex = queueProxyModel.sourceIndex(model.index)
                        Unplayer.Player.queue.currentTrackChanged()
                        if (Unplayer.Player.queue.shuffle) {
                            Unplayer.Player.queue.resetNotPlayedTracks()
                        }
                    }
                }
            }
        }
        model: Unplayer.FilterProxyModel {
            id: queueProxyModel

            filterRole: Unplayer.QueueModel.TitleRole
            sourceModel: Unplayer.QueueModel {
                id: queueModel
                queue: Unplayer.Player.queue
            }
        }

        PullDownMenu {
            MenuItem {
                text: qsTranslate("unplayer", "Clear")
                onClicked: Unplayer.Player.queue.clear()
            }

            SelectionMenuItem {
                text: qsTranslate("unplayer", "Select tracks")
            }

            SearchMenuItem { }
        }

        ListViewPlaceholder {
            listView: listView
            text: qsTranslate("unplayer", "No tracks")
        }

        VerticalScrollDecorator { }
    }
}
