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

import "../components"
import "../models"

Page {
    property alias bottomPanelOpen: selectionPanel.open

    SearchPanel {
        id: searchPanel
    }

    SelectionPanel {
        id: selectionPanel
        selectionText: qsTr("%n artist(s) selected", String(), artistsProxyModel.selectedIndexesCount)

        PushUpMenu {
            AddToQueueMenuItem {
                enabled: artistsProxyModel.selectedIndexesCount !== 0

                onClicked: {
                    player.queue.add(selectionPanel.getTracksForSelectedArtists())
                    player.queue.setCurrentToFirstIfNeeded()
                    selectionPanel.showPanel = false
                }
            }

            AddToPlaylistMenuItem {
                enabled: artistsProxyModel.selectedIndexesCount !== 0
                onClicked: pageStack.push(addToPlaylistPage)

                Component {
                    id: addToPlaylistPage

                    AddToPlaylistPage {
                        tracks: selectionPanel.getTracksForSelectedArtists()
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

        header: PageHeader {
            title: qsTr("Artists")
        }
        delegate: MediaContainerSelectionDelegate {
            id: artistDelegate

            property bool unknownArtist: model.rawArtist === undefined

            function getTracks() {
                return sparqlConnection.select(Unplayer.Utils.tracksSparqlQuery(false,
                                                                                true,
                                                                                model.rawArtist))
            }

            function reloadMediaArt() {
                mediaArt = String()
                mediaArt = Unplayer.Utils.mediaArtForArtist(model.rawArtist)
            }

            title: Theme.highlightText(model.artist, searchPanel.searchText, Theme.highlightColor)
            description: qsTr("%n album(s)", String(), model.albumsCount)
            mediaArt: Unplayer.Utils.mediaArtForArtist(model.rawArtist)
            menu: Component {
                ContextMenu {
                    MenuItem {
                        text: qsTr("All tracks")
                        onClicked: pageStack.push("TracksPage.qml", {
                                                      pageTitle: model.artist,
                                                      allArtists: false,
                                                      artist: model.rawArtist ? model.rawArtist : String()
                                                  })
                    }

                    AddToQueueMenuItem {
                        onClicked: {
                            player.queue.add(getTracks())
                            player.queue.setCurrentToFirstIfNeeded()
                        }
                    }

                    AddToPlaylistMenuItem {
                        onClicked: pageStack.push("AddToPlaylistPage.qml", { tracks: getTracks() })
                    }
                }
            }

            onClicked: {
                if (selectionPanel.showPanel)
                    artistsProxyModel.select(model.index)
                else
                    pageStack.push(artistPage)
            }

            Component {
                id: artistPage
                ArtistPage { }
            }
        }
        model: Unplayer.FilterProxyModel {
            id: artistsProxyModel

            filterRoleName: "artist"
            sourceModel: ArtistsModel {
                id: artistsModel
            }
        }

        PullDownMenu {
            SelectionMenuItem {
                text: qsTr("Select artists")
            }

            SearchMenuItem { }
        }

        ListViewPlaceholder {
            text: qsTr("No artists")
        }

        VerticalScrollDecorator { }
    }
}
