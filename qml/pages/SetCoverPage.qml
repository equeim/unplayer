import QtQuick 2.2
import Sailfish.Silica 1.0

import harbour.unplayer 0.1 as Unplayer

import "../components"

Page {
    property string artist: model.artist
    property string album: model.album

    SilicaListView {
        id: listView

        anchors.fill: parent
        header: Column {
            width: listView.width

            PageHeader {
                title: qsTr("Select JPEG file")
                description: Unplayer.Utils.urlToPath(folderListModel.folder)
            }

            BackgroundItem {
                id: parentDirectoryItem

                visible: folderListModel.folder !== "file:///"
                onClicked: folderListModel.folder = folderListModel.parentFolder

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
                if (model.fileIsDir) {
                    folderListModel.folder = model.filePath
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
                    var iconSource = model.fileIsDir ? "image://theme/icon-m-folder"
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
                text: model.fileName
                color: highlighted ? Theme.highlightColor : Theme.primaryColor
                truncationMode: TruncationMode.Fade
            }
        }
        model: Unplayer.FolderListModel {
            id: folderListModel

            folder: Unplayer.Utils.homeDirectory()
            nameFilters: [ "*.jpeg", "*.jpg" ]
            showDirsFirst: true
        }

        PullDownMenu {
            id: pullDownMenu

            MenuItem {
                function goToScard() {
                    folderListModel.folder = Unplayer.Utils.sdcard()
                    pullDownMenu.activeChanged.disconnect(goToScard)
                }

                text: qsTr("SD card")
                onClicked: pullDownMenu.activeChanged.connect(goToScard)
            }

            MenuItem {
                function goToHome() {
                    folderListModel.folder = Unplayer.Utils.homeDirectory()
                    pullDownMenu.activeChanged.disconnect(goToHome)
                }

                text: qsTr("Home directory")
                onClicked: pullDownMenu.activeChanged.connect(goToHome)
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No files")
        }

        VerticalScrollDecorator { }
    }
}
