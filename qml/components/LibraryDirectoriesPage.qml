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
    id: libraryDirectoriesPage
    property string title
    property alias model: proxyModel.sourceModel
    property bool changed: false

    SelectionPanel {
        id: selectionPanel
        selectionText: qsTranslate("unplayer", "%n directories selected", String(), proxyModel.selectedIndexesCount)

        PushUpMenu {
            MenuItem {
                enabled: proxyModel.hasSelection
                text: qsTranslate("unplayer", "Remove")
                onClicked: {
                    model.removeDirectories(proxyModel.selectedSourceIndexes)
                    changed = true
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
        }
        clip: true

        header: PageHeader {
            title: libraryDirectoriesPage.title
        }
        delegate: ListItem {
            id: delegate

            menu: ContextMenu {
                MenuItem {
                    function remove() {
                        libraryDirectoriesPage.model.removeDirectory(model.index)
                        changed = true
                        delegate.menuOpenChanged.disconnect(remove)
                    }
                    text: qsTranslate("unplayer", "Remove")
                    onClicked: delegate.menuOpenChanged.connect(remove)
                }
            }

            onClicked: {
                if (selectionPanel.showPanel) {
                    proxyModel.select(model.index)
                }
            }

            ListView.onRemove: animateRemoval()

            Label {
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                text: model.directory
                color: highlighted ? Theme.highlightColor : Theme.primaryColor
                truncationMode: TruncationMode.Fade
            }

            Binding {
                target: delegate
                property: "highlighted"
                value: down || menuOpen || proxyModel.isSelected(model.index)
            }

            Connections {
                target: proxyModel
                onSelectionChanged: delegate.highlighted = proxyModel.isSelected(model.index)
            }
        }

        model: Unplayer.FilterProxyModel {
            id: proxyModel
        }

        PullDownMenu {
            SelectionMenuItem {
                text: qsTranslate("unplayer", "Select")
            }

            MenuItem {
                text: qsTranslate("unplayer", "Add directory...")
                onClicked: pageStack.push(filePickerDialogComponent)

                Component {
                    id: filePickerDialogComponent

                    FilePickerDialog {
                        title: qsTranslate("unplayer", "Select directory")
                        showFiles: false
                        onAccepted: {
                            model.addDirectory(filePath)
                            changed = true
                        }
                    }
                }
            }
        }

        ViewPlaceholder {
            enabled: !listView.count
            text: qsTranslate("unplayer", "No directories")
        }

        VerticalScrollDecorator { }
    }
}
