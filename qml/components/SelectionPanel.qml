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

DockedPanel {
    id: selectionPanel

    property alias selectionText: selectionLabel.text
    property bool showPanel: false

    function getTracksForSelectedArtists() {
        var selectedIndexes = listView.model.selectedSourceIndexes()
        var selectedTracks = []
        for (var i = 0, artistsCount = selectedIndexes.length; i < artistsCount; i++) {
            var artistObject = listView.model.sourceModel.get(selectedIndexes[i])
            selectedTracks = selectedTracks.concat(sparqlConnection.select(Unplayer.Utils.tracksSparqlQuery(false,
                                                                                                            true,
                                                                                                            artistObject.artist,
                                                                                                            artistObject.rawArtist === undefined,
                                                                                                            String(),
                                                                                                            false)))
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
                                                                                                            albumObject.artist,
                                                                                                            albumObject.rawArtist === undefined,
                                                                                                            albumObject.album,
                                                                                                            albumObject.rawAlbum === undefined)))
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
        value: showPanel && !Qt.inputMethod.visible && pageStack.currentPage === page
    }

    onOpenChanged: {
        if (!open) {
            if (!Qt.inputMethod.visible && pageStack.currentPage === page) {
                showPanel = false
                listView.model.selectionModel.clear()
            }
        }
    }

    Column {
        id: column

        anchors.verticalCenter: parent.verticalCenter
        width: parent.width

        Label {
            id: selectionLabel
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            width: childrenRect.width + row.x
            height: Math.max(closeButton.height, row.height)

            Row {
                id: row

                property int implicitRowWidth: selectAllButton.implicitWidth + spacing + selectNoneButton.implicitWidth
                property int filledRowWidth: column.width - x - closeButton.width
                property bool fillButtons: implicitRowWidth > filledRowWidth
                property int filledButtonWidth: (filledRowWidth - spacing) / 2

                anchors.verticalCenter: parent.verticalCenter
                x: Theme.paddingMedium
                spacing: Theme.paddingMedium

                Button {
                    id: selectAllButton
                    anchors.verticalCenter: parent.verticalCenter
                    width: row.fillButtons ? row.filledButtonWidth : implicitWidth
                    text: qsTr("All")

                    onClicked: listView.model.selectAll()
                }

                Button {
                    id: selectNoneButton
                    anchors.verticalCenter: parent.verticalCenter
                    width: row.fillButtons ? row.filledButtonWidth : implicitWidth
                    text: qsTr("None")
                    onClicked: listView.model.selectionModel.clear()
                }
            }

            IconButton {
                id: closeButton

                anchors {
                    left: row.right
                    verticalCenter: parent.verticalCenter
                }
                icon.source: "image://theme/icon-m-close"

                onClicked: showPanel = false
            }
        }
    }
}
