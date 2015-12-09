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

    property string pageTitle
    property alias playlistUrl: playlistModel.url

    RemorsePopup {
        id: remorsePopup
    }

    SearchPanel {
        id: searchPanel
    }

    SelectionPanel {
        id: selectionPanel
        selectionText: qsTr("%n track(s) selected", String(), playlistProxyModel.selectedIndexesCount)

        PushUpMenu {
            AddToQueueMenuItem {
                enabled: playlistProxyModel.selectedIndexesCount !== 0

                onClicked: {
                    player.queue.add(selectionPanel.getSelectedTracks())
                    player.queue.setCurrentToFirstIfNeeded()
                    selectionPanel.showPanel = false
                }
            }

            MenuItem {
                enabled: playlistProxyModel.selectedIndexesCount !== 0

                text: qsTr("Remove from playlist")
                onClicked: {
                    var selectedIndexes = playlistProxyModel.selectedSourceIndexes()
                    remorsePopup.execute(qsTr("Removing %n track(s)", String(), playlistProxyModel.selectedIndexesCount),
                                         function() {
                                             playlistModel.removeAtIndexes(selectedIndexes)
                                             Unplayer.PlaylistUtils.removeTracksFromPlaylist(playlistUrl, selectedIndexes)
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
            title: pageTitle
        }
        delegate: BaseTrackDelegate {
            current: model.url === player.queue.currentUrl
            menu: ContextMenu {
                AddToQueueMenuItem {
                    onClicked: {
                        player.queue.add([playlistModel.get(playlistProxyModel.sourceIndex(model.index))])
                        player.queue.setCurrentToFirstIfNeeded()
                    }
                }

                MenuItem {
                    text: qsTr("Remove from playlist")
                    onClicked: remorseAction(qsTr("Removing"), function() {
                        var trackIndex = playlistProxyModel.sourceIndex(model.index)
                        playlistModel.removeAtIndexes([trackIndex])
                        Unplayer.PlaylistUtils.removeTracksFromPlaylist(playlistUrl, [trackIndex])
                    })
                }
            }

            onClicked: {
                if (selectionPanel.showPanel) {
                    playlistProxyModel.select(model.index)
                } else {
                    if (current) {
                        if (!player.playing)
                            player.play()
                        return
                    }

                    player.queue.clear()
                    player.queue.add(playlistProxyModel.getTracks())

                    player.queue.currentIndex = model.index
                    player.queue.currentTrackChanged()
                }
            }
            ListView.onRemove: animateRemoval()
        }
        model: TracksProxyModel {
            id: playlistProxyModel
            sourceModel: Unplayer.PlaylistModel {
                id: playlistModel
            }
        }
        section {
            property: "artist"
            delegate: SectionHeader {
                text: section
            }
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Remove playlist")
                onClicked: remorsePopup.execute(qsTr("Removing playlist"), function() {
                    Unplayer.PlaylistUtils.removePlaylist(playlistModel.url)
                    pageStack.pop()
                })
            }

            SelectionMenuItem {
                text: qsTr("Select tracks")
            }

            SearchMenuItem { }
        }

        ViewPlaceholder {
            enabled: !playlistModel.loaded
            text: qsTr("Loading")
        }

        ListViewPlaceholder {
            enabled: playlistModel.loaded && listView.count === 0
            text: qsTr("No tracks")
        }

        VerticalScrollDecorator { }
    }
}
