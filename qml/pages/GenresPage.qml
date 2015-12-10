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
        selectionText: qsTr("%n genre(s) selected", String(), genresProxyModel.selectedIndexesCount)

        PushUpMenu {
            AddToQueueMenuItem {
                enabled: genresProxyModel.selectedIndexesCount !== 0

                onClicked: {
                    player.queue.add(selectionPanel.getTracksForSelectedGenres())
                    player.queue.setCurrentToFirstIfNeeded()
                    selectionPanel.showPanel = false
                }
            }

            AddToPlaylistMenuItem {
                enabled: genresProxyModel.selectedIndexesCount !== 0
                onClicked: pageStack.push(addToPlaylistPage)

                Component {
                    id: addToPlaylistPage

                    AddToPlaylistPage {
                        tracks: selectionPanel.getTracksForSelectedGenres()
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
            title: qsTr("Genres")
        }
        delegate: MediaContainerSelectionDelegate {
            function getTracks() {
                return sparqlConnection.select(Unplayer.Utils.tracksSparqlQuery(true,
                                                                         true,
                                                                         String(),
                                                                         String(),
                                                                         model.genre))
            }

            title: Theme.highlightText(model.genre, searchPanel.searchText, Theme.highlightColor)
            description: qsTr("%n track(s), %1", String(), model.tracksCount).arg(Unplayer.Utils.formatDuration(model.duration))
            menu: Component {
                ContextMenu {
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
                    genresProxyModel.select(model.index)
                else
                    pageStack.push("TracksPage.qml", {
                                       pageTitle: model.genre,
                                       allArtists: true,
                                       genre: model.genre
                                   })
            }
        }
        model: Unplayer.FilterProxyModel {
            id: genresProxyModel
            sourceModel: GenresModel { }
        }

        PullDownMenu {
            SelectionMenuItem {
                text: qsTr("Select genres")
            }

            SearchMenuItem { }
        }

        ListViewPlaceholder {
            text: qsTr("No genres")
        }

        VerticalScrollDecorator { }
    }
}
