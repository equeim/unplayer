import Sailfish.Silica 1.0

import harbour.unplayer 0.1 as Unplayer

import "../components"
import "../models"

Page {
    id: albumsPage

    SearchListView {
        id: listView

        anchors.fill: parent
        headerTitle: qsTr("Albums")
        delegate: AlbumDelegate {
            description: model.artist
        }
        model: Unplayer.FilterProxyModel {
            filterRoleName: "album"
            sourceModel: AlbumsModel {
                id: albumsModel
                allArtists: true
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
                enabled: listView.count !== 0
                text: qsTr("Search")
                onClicked: listView.showSearchField = true
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No albums")
        }
    }
}


