import Sailfish.Silica 1.0

import harbour.unplayer 0.1 as Unplayer

import "../components"
import "../models"

Page {
    id: page

    SearchListView {
        id: listView

        anchors.fill: parent
        header: ArtistPageHeader { }
        delegate: AlbumDelegate {
            description: qsTr("%n track(s)", String(), model.tracksCount)
        }
        model: Unplayer.FilterProxyModel {
            filterRoleName: "album"
            sourceModel: AlbumsModel {
                allArtists: false
                unknownArtist: artistDelegate.unknownArtist
                artist: model.artist
            }
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Add to playlist")
                onClicked: addTracksToPlaylist()
            }

            MenuItem {
                text: qsTr("Add to queue")
                onClicked: pageStack.push("AddToPlaylistPage.qml", { tracks: getTracks() })
            }

            MenuItem {
                text: qsTr("All tracks")
                onClicked: pageStack.push("AllTracksPage.qml", {
                                              title: model.artist,
                                              unknownArtist: artistDelegate.unknownArtist,
                                              artist: model.artist
                                          })
            }

            MenuItem {
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
