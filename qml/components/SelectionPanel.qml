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

    function getTracksForSelectedArtists() {
        var selectedIndexes = listView.model.selectedSourceIndexes()
        var selectedTracks = []
        for (var i = 0, artistsCount = selectedIndexes.length; i < artistsCount; i++) {
            selectedTracks = selectedTracks.concat(sparqlConnection.select(Unplayer.Utils.tracksSparqlQuery(false,
                                                                                                            true,
                                                                                                            listView.model.sourceModel.get(selectedIndexes[i]).rawArtist)))
        }
        return selectedTracks
    }

    function getTracksForSelectedAlbums() {
        var selectedIndexes = listView.model.selectedSourceIndexes()
        var selectedTracks = []
        for (var i = 0, albumsCount = selectedIndexes.length; i < albumsCount; i++) {
            var albumObject = listView.model.sourceModel.get(selectedIndexes[i])
            selectedTracks = selectedTracks.concat(sparqlConnection.select(Unplayer.Utils.tracksSparqlQuery(false,
                                                                                                            false,
                                                                                                            albumObject.rawArtist,
                                                                                                            albumObject.rawAlbum)))
        }
        return selectedTracks
    }

    function getTracksForSelectedGenres() {
        var selectedIndexes = listView.model.selectedSourceIndexes()
        var selectedTracks = []
        for (var i = 0, albumsCount = selectedIndexes.length; i < albumsCount; i++) {
            selectedTracks = selectedTracks.concat(sparqlConnection.select(Unplayer.Utils.tracksSparqlQuery(true,
                                                                                                            true,
                                                                                                            String(),
                                                                                                            String(),
                                                                                                            listView.model.sourceModel.get(selectedIndexes[i]).genre)))
        }
        return selectedTracks
    }

    function getSelectedTracks() {
        var selectedIndexes = listView.model.selectedSourceIndexes()
        var selectedTracks = []
        for (var i = 0, tracksCount = selectedIndexes.length; i < tracksCount; i++)
            selectedTracks.push(listView.model.sourceModel.get(selectedIndexes[i]))
        return selectedTracks
    }

    width: parent.width
    height: column.height + Theme.paddingLarge
    contentHeight: height

    opacity: open ? 1 : 0

    Behavior on opacity {
        FadeAnimation { }
    }

    Binding {
        target: selectionPanel
        property: "open"
        value: showPanel && !Qt.inputMethod.visible
    }

    onOpenChanged: {
        if (!open && !Qt.inputMethod.visible) {
            showPanel = false
            listView.model.selectionModel.clear()
        }
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
            anchors.horizontalCenter: parent.horizontalCenter
            implicitWidth: Theme.itemSizeHuge * 3 - Theme.horizontalPageMargin * 2
            width: implicitWidth > parent.width ? parent.width : implicitWidth
            height: Math.max(selectionButtons.height, closeButton.height)

            Row {
                id: selectionButtons

                property int buttonWidth: (width - spacing) / 2

                anchors {
                    left: parent.left
                    right: closeButton.left
                    verticalCenter: parent.verticalCenter
                }
                height: childrenRect.height

                spacing: Theme.paddingMedium

                Button {
                    text: qsTr("All")
                    width: selectionButtons.buttonWidth
                    onClicked: listView.model.selectAll()
                }

                Button {
                    width: selectionButtons.buttonWidth
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
