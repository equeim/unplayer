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

Page {
    SearchPanel {
        id: searchPanel
    }

    SelectionPanel {
        id: selectionPanel
        selectionText: qsTranslate("unplayer", "%n file(s) selected", String(), directoryTracksProxyModel.selectedIndexesCount)

        PushUpMenu {
            MenuItem {
                enabled: directoryTracksProxyModel.hasSelection
                text: qsTranslate("unplayer", "Add to queue")
                onClicked: {
                    Unplayer.Player.queue.addTracks(directoryTracksProxyModel.getSelectedTracks())
                    selectionPanel.showPanel = false
                }
            }

            MenuItem {
                enabled: directoryTracksProxyModel.hasSelection
                text: qsTranslate("unplayer", "Add to playlist")
                onClicked: pageStack.push(addToPlaylistPage)

                Component {
                    id: addToPlaylistPage

                    AddToPlaylistPage {
                        tracks: directoryTracksProxyModel.getSelectedTracks()
                        Component.onDestruction: {
                            if (added) {
                                selectionPanel.showPanel = false
                            }
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
            bottomMargin: selectionPanel.visible ? selectionPanel.visibleSize : 0
            topMargin: searchPanel.visibleSize
        }
        clip: true

        header: Column {
            property int oldHeight

            width: listView.width

            onHeightChanged: {
                if (height > oldHeight) {
                    listView.contentY -= (height - oldHeight)
                }
                oldHeight = height
            }

            Component.onCompleted: oldHeight = height

            PageHeader {
                title: qsTranslate("unplayer", "Directories")
                description: directoryTracksModel.directory
            }

            ParentDirectoryItem {
                visible: directoryTracksModel.loaded && directoryTracksModel.directory !== "/"
                onClicked: {
                    if (!selectionPanel.showPanel) {
                        searchPanel.open = false
                        directoryTracksModel.directory = directoryTracksModel.parentDirectory
                    }
                }
            }
        }
        delegate: ListItem {
            id: fileDelegate

            property bool current: model.filePath === Unplayer.Player.queue.currentFilePath

            menu: Component {
                ContextMenu {
                    MenuItem {
                        visible: model.isDirectory
                        text: qsTranslate("unplayer", "Set as default directory")
                        onClicked: Unplayer.Settings.defaultDirectory = model.filePath
                    }

                    MenuItem {
                        visible: !model.isDirectory
                        text: qsTranslate("unplayer", "Track information")
                        onClicked: pageStack.push("TrackInfoPage.qml", { filePath: model.filePath })
                    }

                    MenuItem {
                        visible: !model.isDirectory
                        text: qsTranslate("unplayer", "Add to queue")
                        onClicked: Unplayer.Player.queue.addTrack(model.filePath)
                    }

                    MenuItem {
                        visible: !model.isDirectory
                        text: qsTranslate("unplayer", "Add to playlist")
                        onClicked: pageStack.push("AddToPlaylistPage.qml", { tracks: model.filePath })
                    }
                }
            }
            showMenuOnPressAndHold: !selectionPanel.showPanel

            onClicked: {
                if (selectionPanel.showPanel) {
                    if (!model.isDirectory) {
                        directoryTracksProxyModel.select(model.index)
                    }
                } else {
                    if (model.isDirectory) {
                        searchPanel.open = false
                        directoryTracksModel.directory = model.filePath
                    } else {
                        if (current) {
                            if (!Unplayer.Player.playing) {
                                Unplayer.Player.play()
                            }
                        } else {
                            if (model.isPlaylist) {
                                Unplayer.Player.queue.addTracks(directoryTracksModel.getTrack(directoryTracksProxyModel.sourceIndex(model.index)),
                                                                true)
                            } else {
                                Unplayer.Player.queue.addTracks(directoryTracksModel.getTracks(directoryTracksProxyModel.sourceIndexes, false),
                                                                true,
                                                                model.index - directoryTracksProxyModel.directoriesCount)
                            }
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
                    if (highlighted || current) {
                        iconSource += "?" + Theme.highlightColor
                    }
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
            sourceModel: Unplayer.DirectoryTracksModel {
                id: directoryTracksModel

                onLoadedChanged: {
                    if (loaded) {
                        listView.positionViewAtBeginning()
                    }
                }
            }
        }

        PullDownMenu {
            id: pullDownMenu

            property string directory

            function goBegin(directory) {
                pullDownMenu.directory = directory
                pullDownMenu.activeChanged.connect(goEnd)
            }

            function goEnd() {
                directoryTracksModel.directory = directory
                pullDownMenu.activeChanged.disconnect(goEnd)
            }

            MenuItem {
                text: qsTranslate("unplayer", "Set as default directory")
                onClicked: Unplayer.Settings.defaultDirectory = directoryTracksModel.directory
            }

            MenuItem {
                text: qsTranslate("unplayer", "Default directory")
                onClicked: pullDownMenu.goBegin(Unplayer.Settings.defaultDirectory)
            }

            MenuItem {
                text: qsTranslate("unplayer", "SD card")
                onClicked: pullDownMenu.goBegin(Unplayer.Utils.sdcardPath)
            }

            MenuItem {
                text: qsTranslate("unplayer", "Home directory")
                onClicked: pullDownMenu.goBegin(Unplayer.Utils.homeDirectory)
            }

            SelectionMenuItem {
                enabled: directoryTracksProxyModel.tracksCount || (searchPanel.searchText && directoryTracksProxyModel.tracksCount)
                text: qsTranslate("unplayer", "Select files")
            }

            SearchMenuItem { }
        }

        ListViewPlaceholder {
            enabled: directoryTracksModel.loaded && !listView.count
            text: qsTranslate("unplayer", "No files")
        }

        BusyIndicator {
            anchors.centerIn: parent
            size: BusyIndicatorSize.Large
            running: !directoryTracksModel.loaded
        }

        VerticalScrollDecorator { }
    }
}
