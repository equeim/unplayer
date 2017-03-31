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

DockedPanel {
    id: selectionPanel

    property alias selectionText: selectionLabel.text
    property bool showPanel: false

    width: parent.width
    height: column.height + Theme.paddingLarge
    contentHeight: height

    opacity: open ? 1.0 : 0.0
    Behavior on opacity { FadeAnimation { } }

    Binding {
        target: selectionPanel
        property: "open"
        value: showPanel && !Qt.inputMethod.visible
    }

    onOpenChanged: {
        if (open) {
            visible = true
        } else {
            if (!Qt.inputMethod.visible) {
                showPanel = false
                listView.model.selectionModel.clear()
            }
        }

    }

    onVisibleSizeChanged: {
        if (!visibleSize && !showPanel) {
            visible = false
        }
    }

    Binding {
        target: nowPlayingPanel
        property: "shouldBeClosed"
        value: open
    }

    Column {
        id: column

        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        Label {
            id: selectionLabel

            width: parent.width
            horizontalAlignment: implicitWidth > width ? Text.AlignRight : Text.AlignHCenter
            truncationMode: TruncationMode.Fade
        }

        Item {
            width: parent.width
            height: Math.max(selectionButtons.height, closeButton.height)

            Item {
                id: selectionButtons

                anchors {
                    left: parent.left
                    right: closeButton.left
                    verticalCenter: parent.verticalCenter
                }
                height: childrenRect.height

                Button {
                    anchors {
                        left: parent.left
                        right: parent.horizontalCenter
                        rightMargin: Theme.paddingSmall
                    }
                    text: qsTr("All")
                    onClicked: listView.model.selectAll()
                }

                Button {
                    anchors {
                        left: parent.horizontalCenter
                        leftMargin: Theme.paddingSmall
                        right: parent.right
                    }
                    text: qsTr("None")
                    onClicked: listView.model.selectionModel.clear()
                }
            }

            IconButton {
                id: closeButton

                anchors {
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
                icon.source: "image://theme/icon-m-close"

                onClicked: showPanel = false
            }
        }
    }
}
