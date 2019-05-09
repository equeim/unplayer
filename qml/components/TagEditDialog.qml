/*
 * Unplayer
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
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
    property var files

    canAccept: titleSwitch.checked ||
               artistsListItem.checked ||
               albumArtistsListItem.checked ||
               albumsListItem.checked ||
               yearSwitch.checked ||
               trackNumberSwitch.checked ||
               genresListItem.checked ||
               discNumberSwitch.checked

    onAccepted: {
        var tags = {}

        var getListItemValues = function(listItem) {
            var values = []
            var model = listItem.listModel
            for (var i = 0, max = model.count; i < max; ++i) {
                values.push(model.get(i).value)
            }
            return values
        }

        if (titleSwitch.checked) {
            tags[Unplayer.LibraryUtils.titleTag] = titleTextField.text
        }

        if (artistsListItem.checked) {
            tags[Unplayer.LibraryUtils.artistsTag] = getListItemValues(artistsListItem)
        }

        if (albumArtistsListItem.checked) {
            tags[Unplayer.LibraryUtils.albumArtistsTag] = getListItemValues(albumArtistsListItem)
        }

        if (albumsListItem.checked) {
            tags[Unplayer.LibraryUtils.albumsTag] = getListItemValues(albumsListItem)
        }

        if (yearSwitch.checked) {
            tags[Unplayer.LibraryUtils.yearTag] = yearTextField.text
        }

        if (trackNumberSwitch.checked) {
            tags[Unplayer.LibraryUtils.trackNumberTag] = trackNumberTextField.text
        }

        if (genresListItem.checked) {
            tags[Unplayer.LibraryUtils.genresTag] = getListItemValues(genresListItem)
        }

        if (discNumberSwitch.checked) {
            tags[Unplayer.LibraryUtils.discNumberTag] = discNumberTextField.text
        }

        Unplayer.LibraryUtils.saveTags(files, tags, incrementTrackNumberSwitch.checked)
    }

    Unplayer.TrackInfo {
        id: trackInfo
        filePath: files[0]
    }

    SilicaFlickable {
        id: flickable

        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width

            DialogHeader {
                acceptText: qsTranslate("unplayer", "Save")
                title: qsTranslate("unplayer", "Edit tags")
            }

            Label {
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                }
                height: implicitHeight + Theme.paddingMedium
                color: trackInfo.canReadTags ? Theme.highlightColor : Theme.errorColor
                text: trackInfo.canReadTags ? (files.length > 1 ? qsTranslate("unplayer", "Selected tags will be applied to all %n files", "", files.length)
                                                                : qsTranslate("unplayer", "Only selected tags will be applied"))
                                            : qsTranslate("unplayer", "Can't read tags from this file")
                wrapMode: Text.WordWrap
            }

            Column {
                id: tagsColumn

                opacity: enabled ? 1.0 : 0.4

                function findNextTextField(currentItem) {
                    var reached = false
                    for (var i = 0, max = children.length; i < max; ++i) {
                        var item = children[i]
                        if (reached) {
                            if ((item.objectName === "textField" && item.visible) || item.objectName === "listItem" && item.checked) {
                                return item
                            }
                        } else if (item === currentItem) {
                            reached = true
                        }
                    }
                    return null
                }

                function canFocusNextField(currentItem) {
                    return findNextTextField(currentItem) !== null
                }

                function focusNextTextFieldOrAccept(currentItem) {
                    var field = findNextTextField(currentItem)
                    if (field !== null) {
                        field.forceActiveFocus()
                    } else {
                        accept()
                    }
                }

                width: parent.width
                enabled: trackInfo.canReadTags

                TextSwitch {
                    id: titleSwitch
                    visible: files.length === 1
                    text: qsTranslate("unplayer", "Title")
                }

                TextField {
                    id: titleTextField
                    objectName: "textField"

                    visible: titleSwitch.checked
                    width: parent.width
                    label: qsTranslate("unplayer", "Title")
                    placeholderText: label
                    text: trackInfo.title

                    EnterKey.iconSource: tagsColumn.canFocusNextField(this) ? "image://theme/icon-m-enter-next" : "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: tagsColumn.focusNextTextFieldOrAccept(this)
                }

                TagEditDialogListItem {
                    id: artistsListItem
                    switchText: qsTranslate("unplayer", "Artists")
                    textFieldLabel: qsTranslate("unplayer", "Artist")
                    values: trackInfo.artists
                }

                TagEditDialogListItem {
                    id: albumArtistsListItem
                    switchText: qsTranslate("unplayer", "Album artists")
                    textFieldLabel: qsTranslate("unplayer", "Album artist")
                    values: trackInfo.albumArtists
                }

                TagEditDialogListItem {
                    id: albumsListItem
                    switchText: qsTranslate("unplayer", "Albums")
                    textFieldLabel: qsTranslate("unplayer", "Album")
                    values: trackInfo.albums
                }

                TextSwitch {
                    id: yearSwitch
                    text: qsTranslate("unplayer", "Year")
                }

                TextField {
                    id: yearTextField
                    objectName: "textField"

                    visible: yearSwitch.checked
                    width: parent.width
                    label: qsTranslate("unplayer", "Year")
                    placeholderText: label
                    text: trackInfo.year

                    EnterKey.iconSource: tagsColumn.canFocusNextField(this) ? "image://theme/icon-m-enter-next" : "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: tagsColumn.focusNextTextFieldOrAccept(this)
                }

                TextSwitch {
                    id: trackNumberSwitch
                    text: qsTranslate("unplayer", "Track number")
                }

                TextField {
                    id: trackNumberTextField
                    objectName: "textField"

                    visible: trackNumberSwitch.checked
                    width: parent.width
                    label: qsTranslate("unplayer", "Track number")
                    placeholderText: label
                    text: trackInfo.trackNumber

                    EnterKey.iconSource: tagsColumn.canFocusNextField(this) ? "image://theme/icon-m-enter-next" : "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: tagsColumn.focusNextTextFieldOrAccept(this)
                }

                TextSwitch {
                    id: incrementTrackNumberSwitch
                    anchors {
                        left: parent.left
                        leftMargin: Theme.horizontalPageMargin
                        right: parent.right
                    }
                    visible: trackNumberSwitch.checked && files.length > 1
                    text: qsTranslate("unplayer", "Increment track number for each file")
                }

                TagEditDialogListItem {
                    id: genresListItem
                    switchText: qsTranslate("unplayer", "Genres")
                    textFieldLabel: qsTranslate("unplayer", "Genre")
                    values: trackInfo.genres
                }

                TextSwitch {
                    id: discNumberSwitch
                    text: qsTranslate("unplayer", "Disc number")
                }

                TextField {
                    id: discNumberTextField
                    objectName: "textField"

                    visible: discNumberSwitch.checked
                    width: parent.width
                    label: qsTranslate("unplayer", "Disc number")
                    placeholderText: label
                    text: trackInfo.discNumber

                    EnterKey.iconSource: tagsColumn.canFocusNextField(this) ? "image://theme/icon-m-enter-next" : "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: tagsColumn.focusNextTextFieldOrAccept(this)
                }
            }
        }
    }
}

