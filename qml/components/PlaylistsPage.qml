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

Page {
    RemorsePopup {
        id: remorsePopup
    }

    SearchPanel {
        id: searchPanel
    }

    SelectionPanel {
        id: selectionPanel
        selectionText: qsTranslate("unplayer", "%n playlist(s) selected", String(), playlistsProxyModel.selectedIndexesCount)

        PushUpMenu {
            MenuItem {
                enabled: playlistsProxyModel.hasSelection
                text: qsTranslate("unplayer", "Add to queue")
                onClicked: {
                    Unplayer.Player.queue.addTracks(playlistsModel.getTracksForPlaylists(playlistsProxyModel.selectedSourceIndexes))
                    selectionPanel.showPanel = false
                }
            }

            MenuItem {
                enabled: playlistsProxyModel.hasSelection

                text: qsTranslate("unplayer", "Remove")
                onClicked: {
                    remorsePopup.execute(qsTranslate("unplayer", "Removing %n playlist(s)", String(), playlistsProxyModel.selectedIndexesCount),
                                         function() {
                                             playlistsModel.removePlaylists(playlistsProxyModel.selectedSourceIndexes)
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
            bottomMargin: selectionPanel.visible ? selectionPanel.visibleSize : 0
            topMargin: searchPanel.visibleSize
        }
        clip: true

        header: PageHeader {
            title: qsTranslate("unplayer", "Playlists")
        }
        delegate: MediaContainerSelectionDelegate {
            title: Theme.highlightText(model.name, searchPanel.searchText, Theme.highlightColor)
            description: qsTranslate("unplayer", "%n track(s)", String(), model.tracksCount)
            menu: ContextMenu {
                MenuItem {
                    text: qsTranslate("unplayer", "Add to queue")
                    onClicked: Unplayer.Player.queue.addTracks(Unplayer.PlaylistUtils.getPlaylistTracks(model.filePath))
                }

                MenuItem {
                    text: qsTranslate("unplayer", "Remove")
                    onClicked: remorseAction(qsTranslate("unplayer", "Removing"), function() {
                        Unplayer.PlaylistUtils.removePlaylist(model.filePath)
                    })
                }
            }

            onClicked: {
                if (selectionPanel.showPanel) {
                    playlistsProxyModel.select(model.index)
                } else {
                    pageStack.push(playlistPageComponent)
                }
            }

            Component {
                id: playlistPageComponent
                PlaylistPage {
                    pageTitle: model.name
                    filePath: model.filePath
                }
            }
        }
        model: Unplayer.FilterProxyModel {
            id: playlistsProxyModel

            filterRole: Unplayer.PlaylistsModel.NameRole
            sortEnabled: true
            sortRole: Unplayer.PlaylistsModel.NameRole

            sourceModel: Unplayer.PlaylistsModel {
                id: playlistsModel
            }
        }

        PullDownMenu {
            MenuItem {
                text: qsTranslate("unplayer", "New playlist...")
                onClicked: pageStack.push("NewPlaylistDialog.qml")
            }

            SelectionMenuItem {
                text: qsTranslate("unplayer", "Select playlists")
            }

            SearchMenuItem { }
        }

        ListViewPlaceholder {
            text: qsTranslate("unplayer", "No playlists")
        }

        VerticalScrollDecorator { }
    }
}
