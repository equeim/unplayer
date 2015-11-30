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

Page {
    property alias title: listView.headerTitle
    property alias playlistUrl: playlistModel.url

    RemorsePopup {
        id: remorsePopup
    }

    SearchListView {
        id: listView

        anchors.fill: parent
        delegate: BaseTrackDelegate {
            id: trackDelegate

            current: model.url === player.queue.currentUrl
            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Add to queue")
                    onClicked: {
                        player.queue.add([playlistModel.get(listView.model.sourceIndex(model.index))])
                        if (player.queue.currentIndex === -1) {
                            player.queue.currentIndex = 0
                            player.queue.currentTrackChanged()
                        }
                    }
                }

                MenuItem {
                    text: qsTr("Remove from playlist")
                    onClicked: remorseAction(qsTr("Removing"), function() {
                        var trackIndex = listView.model.sourceIndex(model.index)
                        playlistModel.removeAt(trackIndex)
                        Unplayer.PlaylistUtils.removeTrackFromPlaylist(playlistModel.url, trackIndex)
                    })
                }
            }

            onClicked: {
                if (current) {
                    if (!player.playing)
                        player.play()
                    return
                }

                var trackList = []
                var count = listView.model.count()
                for (var i = 0; i < count; i++) {
                    trackList[i] = playlistModel.get(listView.model.sourceIndex(i))
                }

                player.queue.clear()
                player.queue.add(trackList)

                player.queue.currentIndex = model.index
                player.queue.currentTrackChanged()
            }
            ListView.onRemove: animateRemoval()
        }
        model: Unplayer.FilterProxyModel {
            filterRoleName: "title"
            sourceModel: Unplayer.PlaylistModel {
                id: playlistModel
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

            MenuItem {
                enabled: listView.count !== 0
                text: qsTr("Clear playlist")
                onClicked: remorsePopup.execute(qsTr("Clearing playlist"), function() {
                    playlistModel.clear()
                    Unplayer.PlaylistUtils.clearPlaylist(playlistModel.url)
                })
            }

            MenuItem {
                text: qsTr("Add to queue")
                onClicked: {
                    var tracks = []
                    var count = listView.model.count()
                    for (var i = 0; i < count; i++)
                        tracks[i] = playlistModel.get(listView.model.sourceIndex(i))

                    player.queue.add(tracks)
                    if (player.queue.currentIndex === -1) {
                        player.queue.currentIndex = 0
                        player.queue.currentTrackChanged()
                    }
                }
            }

            SearchPullDownMenuItem { }
        }

        ViewPlaceholder {
            enabled: !playlistModel.loaded
            text: qsTr("Loading")
        }

        ViewPlaceholder {
            enabled: playlistModel.loaded && listView.count === 0
            text: qsTr("No tracks")
        }
    }
}
