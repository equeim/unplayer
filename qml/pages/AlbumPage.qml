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

    SearchPanel {
        id: searchPanel
    }

    SelectionPanel {
        id: selectionPanel
        selectionText: qsTr("%n track(s) selected", String(), tracksProxyModel.selectedIndexesCount)

        PushUpMenu {
            AddToQueueMenuItem {
                enabled: tracksProxyModel.selectedIndexesCount !== 0

                onClicked: {
                    player.queue.add(selectionPanel.getSelectedTracks())
                    player.queue.setCurrentToFirstIfNeeded()
                    selectionPanel.showPanel = false
                }
            }

            AddToPlaylistMenuItem {
                enabled: tracksProxyModel.selectedIndexesCount !== 0
                onClicked: pageStack.push(addToPlaylistPage)

                Component {
                    id: addToPlaylistPage

                    AddToPlaylistPage {
                        tracks: selectionPanel.getSelectedTracks()
                        Component.onDestruction: {
                            if (added)
                                selectionPanel.showPanel = false
                        }
                    }
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

        header: AlbumPageHeader { }
        delegate: LibraryTrackDelegate { }
        model: TracksProxyModel {
            id: tracksProxyModel

            sourceModel: TracksModel {
                id: tracksModel

                allArtists: false
                allAlbums: false

                artist: model.rawArtist ? model.rawArtist : String()
                album: model.rawAlbum ? model.rawAlbum : String()
            }
        }

        PullDownMenu {
            MenuItem {
                visible: !unknownArtist && !unknownAlbum
                text: qsTr("Set cover image")
                onClicked: pageStack.push(setCoverPage)

                Component {
                    id: setCoverPage
                    SetCoverPage { }
                }
            }

            SelectionMenuItem {
                text: qsTr("Select tracks")
            }

            SearchMenuItem { }
        }

        ListViewPlaceholder {
            text: qsTr("No tracks")
        }

        VerticalScrollDecorator { }
    }
}
