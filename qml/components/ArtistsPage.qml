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

import harbour.unplayer 0.1 as Unplayer

Page {
    id: artistsPage

    SearchPanel {
        id: searchPanel
    }

    SelectionPanel {
        id: selectionPanel
        selectionText: qsTranslate("unplayer", "%n artist(s) selected", String(), artistsProxyModel.selectedIndexesCount)

        PushUpMenu {
            MenuItem {
                enabled: artistsProxyModel.hasSelection
                text: qsTranslate("unplayer", "Replace queue")
                onClicked: {
                    Unplayer.Player.queue.addTracksFromLibrary(artistsModel.getTracksForArtists(artistsProxyModel.selectedSourceIndexes), true)
                    selectionPanel.showPanel = false
                }
            }

            MenuItem {
                enabled: artistsProxyModel.hasSelection
                text: qsTranslate("unplayer", "Add to queue")
                onClicked: {
                    Unplayer.Player.queue.addTracksFromLibrary(artistsModel.getTracksForArtists(artistsProxyModel.selectedSourceIndexes))
                    selectionPanel.showPanel = false
                }
            }

            MenuItem {
                enabled: artistsProxyModel.hasSelection
                text: qsTranslate("unplayer", "Add to playlist")
                onClicked: pageStack.push(addToPlaylistPage)

                Component {
                    id: addToPlaylistPage

                    AddToPlaylistPage {
                        tracks: artistsModel.getTracksForArtists(artistsProxyModel.selectedSourceIndexes)
                        Component.onDestruction: {
                            if (added) {
                                selectionPanel.showPanel = false
                            }
                        }
                    }
                }
            }

            MenuItem {
                enabled: artistsProxyModel.hasSelection
                text: qsTranslate("unplayer", "Edit tags")
                onClicked: pageStack.push("TagEditDialog.qml", {files: artistsModel.getTrackPathsForArtists(artistsProxyModel.selectedSourceIndexes)})
            }

            MenuItem {
                enabled: artistsProxyModel.hasSelection
                text: qsTranslate("unplayer", "Remove")
                onClicked: pageStack.push(removeArtistsDialog)

                Component {
                    id: removeArtistsDialog

                    RemoveFilesDialog {
                        title: qsTranslate("unplayer", "Are you sure you want to remove %n selected artists?", String(), artistsProxyModel.selectedIndexesCount)
                        onAccepted: {
                            artistsModel.removeArtists(artistsProxyModel.selectedSourceIndexes, deleteFiles)
                            selectionPanel.showPanel = false
                        }
                    }
                }
            }
        }
    }

    AsyncLoadingListView {
        id: listView

        anchors {
            fill: parent
            bottomMargin: selectionPanel.visible ? selectionPanel.visibleSize : 0
            topMargin: searchPanel.visibleSize
        }
        clip: true

        page: artistsPage
        emptyText: qsTranslate("unplayer", "No artists")

        header: PageHeader {
            title: qsTranslate("unplayer", "Artists")
        }
        delegate: MediaContainerSelectionDelegate {
            id: artistDelegate

            title: Theme.highlightText(model.displayedArtist, searchPanel.searchText, Theme.highlightColor)
            description: qsTranslate("unplayer", "%n album(s)", String(), model.albumsCount)
            mediaArt: model.mediaArt
            menu: Component {
                ContextMenu {
                    MenuItem {
                        text: qsTranslate("unplayer", "All tracks")
                        onClicked: pageStack.push("TracksPage.qml", {pageTitle: model.displayedArtist,
                                                                     queryMode: Unplayer.TracksModel.QueryArtistTracks,
                                                                     artistId: model.artistId})
                    }

                    MenuItem {
                        text: qsTranslate("unplayer", "Add to queue")
                        onClicked: Unplayer.Player.queue.addTracksFromLibrary(artistsModel.getTracksForArtist(artistsProxyModel.sourceIndex(model.index)))
                    }

                    MenuItem {
                        text: qsTranslate("unplayer", "Add to playlist")
                        onClicked: pageStack.push("AddToPlaylistPage.qml", { tracks: artistsModel.getTracksForArtist(artistsProxyModel.sourceIndex(model.index)) })
                    }

                    MenuItem {
                        text: qsTranslate("unplayer", "Edit tags")
                        onClicked: pageStack.push("TagEditDialog.qml", {files: artistsModel.getTrackPathsForArtist(artistsProxyModel.sourceIndex(model.index))})
                    }

                    MenuItem {
                        text: qsTranslate("unplayer", "Remove")
                        onClicked: pageStack.push(removeArtistDialog)
                    }
                }
            }

            onClicked: {
                if (selectionPanel.showPanel) {
                    artistsProxyModel.select(model.index)
                } else {
                    pageStack.push(artistPageComponent)
                }
            }

            Component {
                id: artistPageComponent
                ArtistPage {
                    displayedArtist: model.displayedArtist
                    artistId: model.artistId
                    albumsCount: model.albumsCount
                    tracksCount: model.tracksCount
                    duration: model.duration
                    mediaArt: artistDelegate.mediaArt
                }
            }

            Component {
                id: removeArtistDialog

                RemoveFilesDialog {
                    title: qsTranslate("unplayer", "Are you sure you want to remove this artist?")
                    onAccepted: {
                        artistsModel.removeArtist(artistsProxyModel.sourceIndex(model.index), deleteFiles)
                    }
                }
            }
        }
        model: Unplayer.FilterProxyModel {
            id: artistsProxyModel

            filterRole: Unplayer.ArtistsModel.DisplayedArtistRole
            sourceModel: Unplayer.ArtistsModel {
                id: artistsModel
            }
        }

        PullDownMenu {
            id: pullDownMenu

            MenuItem {
                function toggle() {
                    artistsModel.toggleSortOrder()
                    pullDownMenu.activeChanged.disconnect(toggle)
                }

                text: artistsModel.sortDescending ? qsTranslate("unplayer", "Sort Descending")
                                                  : qsTranslate("unplayer", "Sort Ascending")
                onClicked: pullDownMenu.activeChanged.connect(toggle)
            }

            SelectionMenuItem {
                text: qsTranslate("unplayer", "Select artists")
            }

            SearchMenuItem { }
        }
    }
}
