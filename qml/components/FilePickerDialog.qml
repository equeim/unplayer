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

Dialog {
    id: filePickerDialog

    property string title
    property string fileIcon

    property alias showFiles: directoryContentModel.showFiles
    property alias nameFilters: directoryContentModel.nameFilters

    property string filePath

    canAccept: showFiles ? filePath : true

    Component.onCompleted: {
        if (!showFiles) {
            filePath = directoryContentModel.directory
        }
    }

    SearchPanel {
        id: searchPanel
    }

    SilicaListView {
        id: listView

        property var savedPositions: []
        property bool goingUp

        anchors {
            fill: parent
            topMargin: searchPanel.visibleSize
        }

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

            DialogHeader {
                title: filePickerDialog.title
            }

            Label {
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                }
                height: implicitHeight + Theme.paddingMedium
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.highlightColor
                opacity: 0.6
                horizontalAlignment: implicitWidth > width ? Text.AlignRight : Text.AlignLeft
                truncationMode: TruncationMode.Fade
                text: directoryContentModel.directory
            }

            ParentDirectoryItem {
                id: parentDirectoryItem

                visible: !directoryContentModel.loading && directoryContentModel.directory !== "/"
                onClicked: {
                    searchPanel.open = false
                    if (!showFiles) {
                        filePath = directoryContentModel.parentDirectory
                    }
                    listView.goingUp = true
                    directoryContentModel.directory = directoryContentModel.parentDirectory
                }
            }
        }

        delegate: BackgroundItem {
            onClicked: {
                if (model.isDirectory) {
                    searchPanel.open = false

                    if (!showFiles) {
                        filePickerDialog.filePath = model.filePath
                    }

                    listView.savedPositions.push(listView.contentY + listView.headerItem.height)
                    listView.goingUp = false

                    directoryContentModel.directory = model.filePath
                } else {
                    filePickerDialog.filePath = model.filePath
                    accept()
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
                                                       : fileIcon
                    if (highlighted) {
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
                text: Theme.highlightText(model.name, searchPanel.searchText, Theme.highlightColor)
                color: highlighted ? Theme.highlightColor : Theme.primaryColor
                truncationMode: TruncationMode.Fade
                textFormat: Text.StyledText
            }
        }

        model: Unplayer.DirectoryContentProxyModel {
            filterRole: Unplayer.DirectoryContentModel.NameRole
            sortEnabled: true
            sortRole: Unplayer.DirectoryContentModel.NameRole
            isDirectoryRole: Unplayer.DirectoryContentModel.IsDirectoryRole

            sourceModel: Unplayer.DirectoryContentModel {
                id: directoryContentModel

                onLoaded: {
                    if (listView.goingUp) {
                        if (listView.savedPositions.length) {
                            listView.contentY = (listView.savedPositions.pop() - listView.headerItem.height)
                        } else {
                            listView.positionViewAtBeginning()
                        }
                    } else {
                        listView.positionViewAtBeginning()
                    }
                }
            }
        }

        PullDownMenu {
            id: pullDownMenu

            MenuItem {
                function goToScard() {
                    listView.savedPositions = []
                    listView.goingUp = false
                    directoryContentModel.directory = Unplayer.Utils.sdcardPath
                    pullDownMenu.activeChanged.disconnect(goToScard)
                }

                text: qsTranslate("unplayer", "SD card")
                onClicked: pullDownMenu.activeChanged.connect(goToScard)
            }

            MenuItem {
                function goToHome() {
                    listView.savedPositions = []
                    listView.goingUp = false
                    directoryContentModel.directory = Unplayer.Utils.homeDirectory
                    pullDownMenu.activeChanged.disconnect(goToHome)
                }

                text: qsTranslate("unplayer", "Home directory")
                onClicked: pullDownMenu.activeChanged.connect(goToHome)
            }

            SearchMenuItem { }
        }

        ViewPlaceholder {
            enabled: !directoryContentModel.loading && !listView.count
            text: qsTranslate("unplayer", "No files")
        }

        BusyIndicator {
            anchors.centerIn: parent
            size: BusyIndicatorSize.Large
            running: directoryContentModel.loading
        }

        VerticalScrollDecorator { }
    }
}
