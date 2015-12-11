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

import harbour.unplayer 0.1 as Unplayer

import "../components"
import "../models"

Page {
    property alias bottomPanelOpen: selectionPanel.open

    SearchPanel {
        id: searchPanel
    }

    SelectionPanel {
        id: selectionPanel
        selectionText: qsTr("%n track(s) selected", String(), directoryTracksProxyModel.selectedIndexesCount)

        PushUpMenu {
            AddToQueueMenuItem {
                enabled: directoryTracksProxyModel.selectedIndexesCount !== 0

                onClicked: {
                    player.queue.add(directoryTracksProxyModel.getSelectedTracks())
                    player.queue.setCurrentToFirstIfNeeded()
                    selectionPanel.showPanel = false
                }
            }

            AddToPlaylistMenuItem {
                enabled: directoryTracksProxyModel.selectedIndexesCount !== 0
                onClicked: pageStack.push(addToPlaylistPage)

                Component {
                    id: addToPlaylistPage

                    AddToPlaylistPage {
                        tracks: directoryTracksProxyModel.getSelectedTracks()
                        Component.onDestruction: {
                            if (added)
                                selectionPanel.showPanel = false
                        }
                    }
                }
            }
        }
    }

    SilicaListView {
        id: listView

        anchors {
            fill: parent
            bottomMargin: selectionPanel.visibleSize
            topMargin: searchPanel.visibleSize
        }
        clip: true

        header: Column {
            width: listView.width

            PageHeader {
                title: qsTr("Directories")
                description: directoryTracksModel.directory
            }

            BackgroundItem {
                id: parentDirectoryItem

                visible: directoryTracksModel.directory !== "/"
                onClicked: {
                    if (!selectionPanel.showPanel) {
                        searchPanel.open = false
                        directoryTracksModel.directory = directoryTracksModel.parentDirectory
                    }
                }

                Row {
                    anchors {
                        left: parent.left
                        leftMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    spacing: Theme.paddingMedium

                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        asynchronous: true
                        source: parentDirectoryItem.highlighted ? "image://theme/icon-m-folder?" + Theme.highlightColor :
                                                                  "image://theme/icon-m-folder"
                    }

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: ".."
                        color: parentDirectoryItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                    }
                }
            }
        }
        delegate: ListItem {
            id: fileDelegate

            property bool current: model.url === player.queue.currentUrl

            menu: Component {
                ContextMenu {
                    MenuItem {
                        text: qsTr("Track information")
                        onClicked: pageStack.push("../pages/TrackInfoPage.qml", { trackUrl: model.url })
                    }

                    AddToQueueMenuItem {
                        onClicked: {
                            player.queue.add([directoryTracksModel.get(directoryTracksProxyModel.sourceIndex(model.index))])
                            player.queue.setCurrentToFirstIfNeeded()
                        }
                    }

                    AddToPlaylistMenuItem {
                        onClicked: pageStack.push("AddToPlaylistPage.qml", { tracks: model.url })
                    }
                }
            }
            showMenuOnPressAndHold: !model.isDirectory && !selectionPanel.showPanel

            onClicked: {
                if (selectionPanel.showPanel) {
                    if (!model.isDirectory)
                        directoryTracksProxyModel.select(model.index)
                } else {
                    if (model.isDirectory) {
                        searchPanel.open = false
                        directoryTracksModel.directory = Unplayer.Utils.urlToPath(model.url)
                    } else {
                        if (current) {
                            if (!player.playing)
                                player.play()
                        } else {
                            player.queue.clear()

                            var tracks = directoryTracksProxyModel.getTracks()
                            player.queue.add(tracks)

                            var trackUrl = model.url
                            for (var i = 0, tracksCount = tracks.length; i < tracksCount; i++) {
                                if (tracks[i].url === model.url) {
                                    player.queue.currentIndex = i
                                    break
                                }
                            }

                            player.queue.currentTrackChanged()
                        }
                    }
                }
            }

            Binding {
                target: fileDelegate
                property: "highlighted"
                value: down || menuOpen || directoryTracksProxyModel.isSelected(model.index)
            }

            Connections {
                target: directoryTracksProxyModel
                onSelectionChanged: fileDelegate.highlighted = directoryTracksProxyModel.isSelected(model.index)
            }

            Image {
                id: icon

                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                asynchronous: true
                source: {
                    var iconSource = model.isDirectory ? "image://theme/icon-m-folder"
                                                       : "image://theme/icon-m-music"
                    if (highlighted || current)
                        iconSource += "?" + Theme.highlightColor
                    return iconSource
                }
            }

            Label {
                anchors {
                    left: icon.right
                    leftMargin: Theme.paddingMedium
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                text: Theme.highlightText(model.fileName, searchPanel.searchText, Theme.highlightColor)
                color: highlighted || current ? Theme.highlightColor : Theme.primaryColor
                truncationMode: TruncationMode.Fade
            }
        }
        model: Unplayer.DirectoryTracksProxyModel {
            id: directoryTracksProxyModel

            filterRoleName: "fileName"
            sourceModel: Unplayer.DirectoryTracksModel {
                id: directoryTracksModel
            }
        }

        PullDownMenu {
            id: pullDownMenu

            MenuItem {
                function goToScard() {
                    directoryTracksModel.directory = Unplayer.Utils.sdcard()
                    pullDownMenu.activeChanged.disconnect(goToScard)
                }

                text: qsTr("SD card")
                onClicked: pullDownMenu.activeChanged.connect(goToScard)
            }

            MenuItem {
                function goToHome() {
                    directoryTracksModel.directory = Unplayer.Utils.homeDirectory()
                    pullDownMenu.activeChanged.disconnect(goToHome)
                }

                text: qsTr("Home directory")
                onClicked: pullDownMenu.activeChanged.connect(goToHome)
            }

            SelectionMenuItem {
                enabled: directoryTracksProxyModel.tracksCount !== 0 || (searchPanel.searchText.length !== 0 && directoryTracksModel.tracksCount !== 0)
                text: qsTr("Select tracks")
            }

            SearchMenuItem { }
        }

        ViewPlaceholder {
            enabled: !directoryTracksModel.loaded
            text: qsTr("Loading")
        }

        ListViewPlaceholder {
            text: qsTr("No files")
        }

        VerticalScrollDecorator { }
    }
}
