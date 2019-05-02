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

Column {
    id: listItemColumn
    objectName: "listItem"

    property alias checked: enableSwitch.checked
    property alias switchText: enableSwitch.text
    property string textFieldLabel
    property alias listModel: listModel
    property var values

    function forceActiveFocus() {
        if (repeater.count > 0) {
            repeater.itemAt(0).textField.forceActiveFocus()
        }
    }

    Component.onCompleted: {
        for (var i = 0, max = values.length; i < max; ++i) {
            listModel.append({"value": values[i]})
        }
    }

    width: parent.width

    TextSwitch {
        id: enableSwitch
    }

    Repeater {
        id: repeater

        model: ListModel {
            id: listModel
        }

        delegate: Row {
            property alias textField: textField
            property bool lastItem: (index === listModel.count - 1)

            visible: enableSwitch.checked

            TextField {
                id: textField

                property bool canFocusNext: !lastItem || tagsColumn.canFocusNextField(listItemColumn.parent)

                width: listItemColumn.width - removeButton.width - Theme.horizontalPageMargin

                label: textFieldLabel
                placeholderText: textFieldLabel
                text: value

                onTextChanged: value = text

                EnterKey.iconSource: canFocusNext ? "image://theme/icon-m-enter-next" : "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: {
                    if (lastItem) {
                        tagsColumn.focusNextTextFieldOrAccept(listItemColumn.parent)
                    } else {
                        repeater.itemAt(index + 1).textField.forceActiveFocus()
                    }
                }
            }

            IconButton {
                id: removeButton
                anchors.verticalCenter: parent.verticalCenter
                icon.source: "image://theme/icon-m-remove"
                onClicked: listModel.remove(index)
            }
        }
    }

    Button {
        anchors.horizontalCenter: parent.horizontalCenter
        visible: enableSwitch.checked
        preferredWidth: Theme.buttonWidthLarge
        text: qsTranslate("unplayer", "Add")
        onClicked: {
            listModel.append({"value": ""})
            flickable.contentY += repeater.itemAt(repeater.count - 1).height
            repeater.itemAt(repeater.count - 1).textField.forceActiveFocus()
        }
    }
}
