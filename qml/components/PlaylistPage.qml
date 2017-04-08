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

Page {
    property string pageTitle
    property alias filePath: playlistModel.filePath

    RemorsePopup {
        id: remorsePopup
    }

    SearchPanel {
        id: searchPanel
    }

    SelectionPanel {
        id: selectionPanel
        selectionText: qsTranslate("unplayer", "%n track(s) selected", String(), playlistProxyModel.selectedIndexesCount)

        PushUpMenu {
            MenuItem {
                enabled: playlistProxyModel.hasSelection
                text: qsTranslate("unplayer", "Add to queue")

                onClicked: {
                    Unplayer.Player.queue.addTracks(playlistModel.getTracks(playlistProxyModel.selectedSourceIndexes))
                    selectionPanel.showPanel = false
                }
            }

            MenuItem {
                enabled: playlistProxyModel.hasSelection

                text: qsTranslate("unplayer", "Remove from playlist")
                onClicked: {
                    var selectedIndexes = playlistProxyModel.selectedSourceIndexes
                    remorsePopup.execute(qsTranslate("unplayer", "Removing %n track(s)", String(), playlistProxyModel.selectedIndexesCount),
                                         function() {
                                             playlistModel.removeTracks(playlistProxyModel.selectedSourceIndexes)
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
            title: pageTitle
        }
        delegate: BaseTrackDelegate {
            showArtist: !model.inLibrary
            showArtistAndAlbum: model.inLibrary
            showDuration: model.hasDuration
            current: model.filePath === Unplayer.Player.queue.currentFilePath
            menu: ContextMenu {
                MenuItem {
                    text: qsTranslate("unplayer", "Track information")
                    onClicked: pageStack.push("TrackInfoPage.qml", { filePath: model.filePath })
                }

                MenuItem {
                    text: qsTranslate("unplayer", "Add to queue")
                    onClicked: Unplayer.Player.queue.addTrack(model.filePath)
                }

                MenuItem {
                    text: qsTranslate("unplayer", "Remove from playlist")
                    onClicked: remorseAction(qsTranslate("unplayer", "Removing"), function() {
                        playlistModel.removeTrack(playlistProxyModel.sourceIndex(model.index))
                    })
                }
            }

            onClicked: {
                if (selectionPanel.showPanel) {
                    playlistProxyModel.select(model.index)
                } else {
                    if (current) {
                        if (!Unplayer.Player.playing) {
                            Unplayer.Player.play()
                        }
                    } else {
                        Unplayer.Player.queue.addTracks(playlistModel.getTracks(playlistProxyModel.sourceIndexes),
                                                        true,
                                                        model.index)
                    }
                }
            }
            ListView.onRemove: animateRemoval()
        }
        model: Unplayer.FilterProxyModel {
            id: playlistProxyModel

            filterRole: Unplayer.PlaylistModel.TitleRole
            sourceModel: Unplayer.PlaylistModel {
                id: playlistModel
            }
        }

        PullDownMenu {
            MenuItem {
                text: qsTranslate("unplayer", "Remove playlist")
                onClicked: remorsePopup.execute(qsTranslate("unplayer", "Removing playlist"), function() {
                    Unplayer.PlaylistUtils.removePlaylist(playlistModel.url)
                    pageStack.pop()
                })
            }

            SelectionMenuItem {
                text: qsTranslate("unplayer", "Select tracks")
            }

            SearchMenuItem { }
        }

        ListViewPlaceholder {
            enabled: !listView.count
            text: qsTranslate("unplayer", "No tracks")
        }

        VerticalScrollDecorator { }
    }
}
