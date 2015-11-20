import Sailfish.Silica 1.0

import harbour.unplayer 0.1 as Unplayer

import "../components"
import "../models"

Page {
    id: page

    property var parentPage
    property var tracks

    SearchListView {
        id: listView

        anchors.fill: parent
        headerTitle: qsTr("Add to playlist")
        delegate: MediaContainerListItem {
            title: Theme.highlightText(model.title, listView.searchFieldText.trim(), Theme.highlightColor)
            description: model.count === undefined ? String() :
                                                     qsTr("%n track(s)", String(), model.count)
            onClicked: {
                Unplayer.Utils.addTracksToPlaylist(model.url, tracks)
                pageStack.pop()
            }
        }
        model: Unplayer.FilterProxyModel {
            filterRoleName: "title"
            sourceModel: PlaylistsModel { }
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("New playlist...")
                onClicked: pageStack.push("NewPlaylistDialog.qml", {
                                       acceptDestination: parentPage,
                                       acceptDestinationAction: PageStackAction.Pop,
                                       tracks: page.tracks
                                   })
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
