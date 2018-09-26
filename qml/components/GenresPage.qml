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
    Binding {
        target: modalDialog
        property: "active"
        value: genresModel.removingFiles
    }

    Binding {
        target: modalDialog
        property: "text"
        value: qsTranslate("unplayer", "Removing albums...")
    }

    SearchPanel {
        id: searchPanel
    }

    SelectionPanel {
        id: selectionPanel
        selectionText: qsTranslate("unplayer", "%n genre(s) selected", String(), genresProxyModel.selectedIndexesCount)

        PushUpMenu {
            MenuItem {
                enabled: genresProxyModel.hasSelection
                text: qsTranslate("unplayer", "Add to queue")
                onClicked: {
                    Unplayer.Player.queue.addTracksFromLibrary(genresModel.getTracksForGenres(genresProxyModel.selectedSourceIndexes))
                    selectionPanel.showPanel = false
                }
            }

            MenuItem {
                enabled: genresProxyModel.hasSelection
                text: qsTranslate("unplayer", "Add to playlist")
                onClicked: pageStack.push(addToPlaylistPageComponent)

                Component {
                    id: addToPlaylistPageComponent

                    AddToPlaylistPage {
                        tracks: genresModel.getTracksForGenres(genresProxyModel.selectedSourceIndexes)
                        Component.onDestruction: {
                            if (added) {
                                selectionPanel.showPanel = false
                            }
                        }
                    }
                }
            }

            MenuItem {
                enabled: genresProxyModel.hasSelection
                text: qsTranslate("unplayer", "Remove")
                onClicked: pageStack.push(removeGenresDialog)

                Component {
                    id: removeGenresDialog

                    RemoveFilesDialog {
                        title: qsTranslate("unplayer", "Are you sure you want to remove %n selected genres?", String(), genresProxyModel.selectedIndexesCount)
                        onAccepted: {
                            genresModel.removeGenres(genresProxyModel.selectedSourceIndexes, deleteFiles)
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
            bottomMargin: selectionPanel.visible ? selectionPanel.visibleSize : 0
            topMargin: searchPanel.visibleSize
        }
        clip: true

        header: PageHeader {
            title: qsTranslate("unplayer", "Genres")
        }
        delegate: MediaContainerSelectionDelegate {
            title: Theme.highlightText(model.genre, searchPanel.searchText, Theme.highlightColor)
            description: qsTranslate("unplayer", "%n track(s), %1", String(), model.tracksCount).arg(Unplayer.Utils.formatDuration(model.duration))
            menu: Component {
                ContextMenu {
                    MenuItem {
                        text: qsTranslate("unplayer", "Add to queue")
                        onClicked: Unplayer.Player.queue.addTracksFromLibrary(genresModel.getTracksForGenre(genresProxyModel.sourceIndex(model.index)))
                    }

                    MenuItem {
                        text: qsTranslate("unplayer", "Add to playlist")
                        onClicked: pageStack.push("AddToPlaylistPage.qml", { tracks: genresModel.getTracksForGenre(genresProxyModel.sourceIndex(model.index)) })
                    }

                    MenuItem {
                        text: qsTranslate("unplayer", "Remove")
                        onClicked: pageStack.push(removeGenreDialog)
                    }
                }
            }

            onClicked: {
                if (selectionPanel.showPanel) {
                    genresProxyModel.select(model.index)
                } else {
                    pageStack.push(tracksPageComponent)
                }
            }

            Component {
                id: tracksPageComponent
                TracksPage {
                    pageTitle: model.genre
                    allArtists: true
                    genre: model.genre
                }
            }

            Component {
                id: removeGenreDialog

                RemoveFilesDialog {
                    title: qsTranslate("unplayer", "Are you sure you want to remove this genre?")
                    onAccepted: {
                        genresModel.removeGenre(genresProxyModel.sourceIndex(model.index), deleteFiles)
                    }
                }
            }
        }
        model: Unplayer.FilterProxyModel {
            id: genresProxyModel
            sourceModel: Unplayer.GenresModel {
                id: genresModel
            }
        }

        PullDownMenu {
            id: pullDownMenu

            MenuItem {
                function toggle() {
                    genresModel.toggleSortOrder()
                    pullDownMenu.activeChanged.disconnect(toggle)
                }

                text: genresModel.sortDescending ? qsTranslate("unplayer", "Sort Descending")
                                                 : qsTranslate("unplayer", "Sort Ascending")
                onClicked: pullDownMenu.activeChanged.connect(toggle)
            }

            SelectionMenuItem {
                text: qsTranslate("unplayer", "Select genres")
            }

            SearchMenuItem { }
        }

        ListViewPlaceholder {
            text: qsTranslate("unplayer", "No genres")
        }

        VerticalScrollDecorator { }
    }
}
