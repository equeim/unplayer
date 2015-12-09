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

import "../components"
import "../models"

Page {
    id: page

    property alias bottomPanelOpen: selectionPanel.open

    function getTracksForSelectedPlaylists() {
        var selectedIndexes = playlistsProxyModel.selectedSourceIndexes()
        var selectedTracks = []
        for (var i = 0, playlistsCount = selectedIndexes.length; i < playlistsCount; i++)
            selectedTracks = selectedTracks.concat(Unplayer.PlaylistUtils.syncParsePlaylist(playlistsModel.get(selectedIndexes[i]).url))
        return selectedTracks
    }

    Connections {
        target: Unplayer.PlaylistUtils
        onPlaylistsChanged: playlistsModel.reload()
    }

    RemorsePopup {
        id: remorsePopup
    }

    SearchPanel {
        id: searchPanel
    }

    SelectionPanel {
        id: selectionPanel
        selectionText: qsTr("%n playlist(s) selected", String(), playlistsProxyModel.selectedIndexesCount)

        PushUpMenu {
            AddToQueueMenuItem {
                enabled: playlistsProxyModel.selectedIndexesCount !== 0

                onClicked: {
                    player.queue.add(getTracksForSelectedPlaylists())
                    player.queue.setCurrentToFirstIfNeeded()
                    selectionPanel.showPanel = false
                }
            }

            MenuItem {
                enabled: playlistsProxyModel.selectedIndexesCount !== 0

                text: qsTr("Remove")
                onClicked: {
                    remorsePopup.execute(qsTr("Removing %n playlist(s)", String(), playlistsProxyModel.selectedIndexesCount),
                                         function() {
                                             var selectedIndexes = playlistsProxyModel.selectedSourceIndexes()

                                             for (var i = 0, playlistsCount = selectedIndexes.length; i < playlistsCount; i++)
                                                 Unplayer.PlaylistUtils.removePlaylist(playlistsModel.get(selectedIndexes[i]).url)

                                             selectionPanel.showPanel = false
                                         })
                }
            }
        }
    }

    SilicaListView {
        id: listView

        anchors {
            fill: parent
            bottomMargin: selectionPanel.visibleSize
            topMargin: searchPanel.visibleSize
        }
        clip: true

        header: PageHeader {
            title: qsTr("Playlists")
        }
        delegate: MediaContainerSelectionDelegate {
            title: Theme.highlightText(model.title, selectionPanel.searchText, Theme.highlightColor)
            description: model.tracksCount === undefined ? String() :
                                                           qsTr("%n track(s)", String(), model.tracksCount)
            menu: Component {
                ContextMenu {
                    AddToQueueMenuItem {
                        onClicked: {
                            player.queue.add(Unplayer.PlaylistUtils.syncParsePlaylist(model.url))
                            player.queue.setCurrentToFirstIfNeeded()
                        }
                    }

                    MenuItem {
                        text: qsTr("Remove")
                        onClicked: remorseAction(qsTr("Removing"), function() {
                            Unplayer.PlaylistUtils.removePlaylist(model.url)
                        })
                    }
                }
            }

            onClicked: {
                if (selectionPanel.showPanel) {
                    playlistsProxyModel.select(model.index)
                } else {
                    pageStack.push("PlaylistPage.qml", {
                                       pageTitle: model.title,
                                       playlistUrl: model.url
                                   })
                }
            }
        }
        model: Unplayer.FilterProxyModel {
            id: playlistsProxyModel

            filterRoleName: "title"
            sourceModel: PlaylistsModel {
                id: playlistsModel
            }
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("New playlist...")
                onClicked: pageStack.push("NewPlaylistDialog.qml")
            }

            SelectionMenuItem {
                text: qsTr("Select playlists")
            }

            SearchMenuItem { }
        }

        ListViewPlaceholder {
            text: qsTr("No playlists")
        }

        VerticalScrollDecorator { }
    }
}
