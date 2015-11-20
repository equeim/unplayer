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
        delegate: MediaContainerListItem {
            id: albumDelegate

            property bool unknownArtist: model.rawArtist === undefined
            property bool unknownAlbum: model.rawAlbum === undefined

            title: Theme.highlightText(model.album, listView.searchFieldText.trim(), Theme.highlightColor)
            description: model.artist
            mediaArt: {
                if (unknownArtist || unknownAlbum)
                    return String()
                return Unplayer.Utils.mediaArt(model.artist, model.album)
            }

            onClicked: pageStack.push("AlbumPage.qml", {
                                          allArtists: true,
                                          unknownAlbum: albumDelegate.unknownAlbum,
                                          album: model.album,
                                          artist: model.artist,
                                          tracksCount: model.tracksCount,
                                          duration: model.duration
                                      })
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


