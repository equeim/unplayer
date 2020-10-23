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
    id: tracksPage

    property string pageTitle

    property alias queryMode: tracksModel.queryMode
    property alias artistId: tracksModel.artistId
    property alias genreId: tracksModel.genreId

    readonly property bool singleArtist: {
        switch (queryMode) {
        case Unplayer.TracksModel.QueryArtistTracks:
        case Unplayer.TracksModel.QueryAlbumTracksForSingleArtist:
            return true
        default:
            return false;
        }
    }

    SearchPanel {
        id: searchPanel
    }

    RemorsePopup {
        id: remorsePopup
    }

    SelectionPanel {
        id: selectionPanel
        selectionText: qsTranslate("unplayer", "%n track(s) selected", String(), tracksProxyModel.selectedIndexesCount)

        PushUpMenu {
            MenuItem {
                enabled: tracksProxyModel.hasSelection
                text: qsTranslate("unplayer", "Replace queue")
                onClicked: {
                    Unplayer.Player.queue.addTracksFromLibrary(tracksModel.getTracks(tracksProxyModel.selectedSourceIndexes), true)
                    selectionPanel.showPanel = false
                }
            }

            MenuItem {
                enabled: tracksProxyModel.hasSelection
                text: qsTranslate("unplayer", "Add to queue")
                onClicked: {
                    Unplayer.Player.queue.addTracksFromLibrary(tracksModel.getTracks(tracksProxyModel.selectedSourceIndexes))
                    selectionPanel.showPanel = false
                }
            }

            MenuItem {
                enabled: tracksProxyModel.hasSelection
                text: qsTranslate("unplayer", "Add to playlist")
                onClicked: pageStack.push(addToPlaylistPage)

                Component {
                    id: addToPlaylistPage

                    AddToPlaylistPage {
                        tracks: tracksModel.getTracks(tracksProxyModel.selectedSourceIndexes)
                        Component.onDestruction: {
                            if (added) {
                                selectionPanel.showPanel = false
                            }
                        }
                    }
                }
            }

            MenuItem {
                enabled: tracksProxyModel.hasSelection
                text: qsTranslate("unplayer", "Edit tags")
                onClicked: pageStack.push("TagEditDialog.qml", {files: tracksModel.getTrackPaths(tracksProxyModel.selectedSourceIndexes)})
            }

            MenuItem {
                enabled: tracksProxyModel.hasSelection
                text: qsTranslate("unplayer", "Remove")
                onClicked: pageStack.push(removeTracksDialog)

                Component {
                    id: removeTracksDialog

                    RemoveFilesDialog {
                        title: qsTranslate("unplayer", "Are you sure you want to remove %n selected tracks?", String(), tracksProxyModel.selectedIndexesCount)
                        onAccepted: {
                            tracksModel.removeTracks(tracksProxyModel.selectedSourceIndexes, deleteFiles)
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

        page: tracksPage
        emptyText: qsTranslate("unplayer", "No tracks")

        header: PageHeader {
            title: pageTitle
        }
        delegate: LibraryTrackDelegate {
            showArtistAndAlbum: !singleArtist
            showAlbum: !showArtistAndAlbum
        }
        model: Unplayer.FilterProxyModel {
            id: tracksProxyModel

            filterRole: Unplayer.TracksModel.TitleRole
            sourceModel: Unplayer.TracksModel {
                id: tracksModel
            }
        }
        section {
            property: {
                if (tracksModel.sortMode === Unplayer.TracksModelSortMode.Artist_AlbumTitle ||
                        tracksModel.sortMode === Unplayer.TracksModelSortMode.Artist_AlbumYear) {
                    if (singleArtist) {
                        return "album"
                    }
                    return "artist"
                }
                return String()
            }
            delegate: SectionHeader {
                text: section
            }
        }

        PullDownMenu {
            MenuItem {
                text: qsTranslate("unplayer", "Sort")
                onClicked: pageStack.push("AllTracksSortPage.qml", {tracksModel: tracksModel})
            }

            SelectionMenuItem {
                text: qsTranslate("unplayer", "Select tracks")
            }

            SearchMenuItem { }
        }
    }
}
