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
    property string artist: model.artist
    property string album: model.album

    SearchPanel {
        id: searchPanel
    }

    SilicaListView {
        id: listView

        anchors {
            fill: parent
            topMargin: searchPanel.visibleSize
        }
        clip: true
        header: Column {
            width: listView.width

            PageHeader {
                title: qsTr("Select image")
                description: filePickerModel.directory
            }

            BackgroundItem {
                id: parentDirectoryItem

                visible: filePickerModel.directory !== "/"
                onClicked: {
                    searchPanel.open = false
                    filePickerModel.directory = filePickerModel.parentDirectory
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
        delegate: BackgroundItem {
            onClicked: {
                if (model.isDirectory) {
                    searchPanel.open = false
                    filePickerModel.directory = model.filePath
                } else {
                    Unplayer.Utils.setMediaArt(model.filePath, artist, album)
                    reloadMediaArt()
                    pageStack.pop()
                }
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
                                                       : "image://theme/icon-m-image"
                    if (highlighted)
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
                color: highlighted ? Theme.highlightColor : Theme.primaryColor
                truncationMode: TruncationMode.Fade
            }
        }
        model: Unplayer.FilterProxyModel {
            id: filePickerProxyModel

            filterRoleName: "fileName"
            sourceModel: Unplayer.FilePickerModel {
                id: filePickerModel
                nameFilters: Unplayer.Utils.imageNameFilters()
            }
        }

        PullDownMenu {
            id: pullDownMenu

            MenuItem {
                function goToScard() {
                    filePickerModel.directory = Unplayer.Utils.sdcard()
                    pullDownMenu.activeChanged.disconnect(goToScard)
                }

                text: qsTr("SD card")
                onClicked: pullDownMenu.activeChanged.connect(goToScard)
            }

            MenuItem {
                function goToHome() {
                    filePickerModel.directory = Unplayer.Utils.homeDirectory()
                    pullDownMenu.activeChanged.disconnect(goToHome)
                }

                text: qsTr("Home directory")
                onClicked: pullDownMenu.activeChanged.connect(goToHome)
            }

            SearchMenuItem { }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No files")
        }

        VerticalScrollDecorator { }
    }
}

