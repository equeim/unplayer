import QtQuick 2.2
import Sailfish.Silica 1.0

import harbour.unplayer 0.1 as Unplayer

import "../components"
import "../models"

Page {
    Connections {
        target: Unplayer.PlaylistUtils
        onPlaylistsChanged: playlistsModel.reload()
    }

    SearchListView {
        id: listView

        anchors.fill: parent
        headerTitle: qsTr("Playlists")
        delegate: MediaContainerListItem {
            title: Theme.highlightText(model.title, listView.searchFieldText.trim(), Theme.highlightColor)
            description: model.tracksCount === undefined ? String() :
                                                           qsTr("%n track(s)", String(), model.tracksCount)
            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Remove")
                    onClicked: remorseAction(qsTr("Removing"), function() {
                        Unplayer.PlaylistUtils.removePlaylist(model.url)
                    })
                }
            }

            onClicked: pageStack.push("PlaylistPage.qml", {
                                          title: model.title,
                                          playlistUrl: model.url
                                      })
        }
        model: Unplayer.FilterProxyModel {
            id: playlistsProxyModel

            filterRoleName: "title"
            sourceModel: PlaylistsModel {
                id: playlistsModel
            }
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("New playlist...")
                onClicked: pageStack.push("NewPlaylistDialog.qml")
            }

            MenuItem {
                enabled: listView.count !== 0
                text: qsTr("Search")
                onClicked: listView.showSearchField = true
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No playlists")
        }
    }
}
