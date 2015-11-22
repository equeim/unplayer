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

            MenuItem {
                text: qsTr("Search")
                onClicked: listView.showSearchField = true
            }
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
