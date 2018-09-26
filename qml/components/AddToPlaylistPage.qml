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
    id: page

    property var tracks
    property bool added

    SearchPanel {
        id: searchPanel
    }

    SilicaListView {
        id: listView

        anchors {
            fill: parent
            topMargin: searchPanel.visibleSize
        }
        clip: true

        header: PageHeader {
            title: qsTranslate("unplayer", "Add to playlist")
        }
        delegate: MediaContainerListItem {
            title: Theme.highlightText(model.name, searchPanel.searchText, Theme.highlightColor)
            description: qsTranslate("unplayer", "%n track(s)", String(), model.tracksCount)
            onClicked: {
                if (Array.isArray(tracks)) {
                    Unplayer.PlaylistUtils.addTracksToPlaylistFromFilesystem(model.filePath, tracks)
                } else {
                    Unplayer.PlaylistUtils.addTracksToPlaylistFromLibrary(model.filePath, tracks)
                }

                added = true
                pageStack.pop()
            }
        }
        model: Unplayer.FilterProxyModel {
            filterRole: Unplayer.PlaylistsModel.NameRole
            sourceModel: Unplayer.PlaylistsModel { }
        }

        PullDownMenu {
            MenuItem {
                text: qsTranslate("unplayer", "New playlist...")
                onClicked: pageStack.push(newPlaylistDialog, { acceptDestination: pageStack.previousPage() })

                Component {
                    id: newPlaylistDialog

                    NewPlaylistDialog {
                        acceptDestinationAction: PageStackAction.Pop
                        tracks: page.tracks
                        onAccepted: {
                            console.log(acceptDestination)
                            added = true
                        }
                    }
                }
            }

            SearchMenuItem { }
        }

        ListViewPlaceholder {
            text: qsTranslate("unplayer", "No playlists")
        }

        VerticalScrollDecorator { }
    }
}
