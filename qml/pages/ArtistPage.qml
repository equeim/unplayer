import Sailfish.Silica 1.0

import harbour.unplayer 0.1 as Unplayer

import "../components"
import "../models"

Page {
    id: artistPage

    property bool unknownArtist
    property string artist

    property int albumsCount
    property int tracksCount
    property int duration

    SearchListView {
        id: listView

        anchors.fill: parent
        header: ArtistPageHeader { }
        delegate: MediaContainerListItem {
            id: albumDelegate

            property bool unknownAlbum: model.rawAlbum === undefined

            title: Theme.highlightText(model.album, listView.searchFieldText.trim(), Theme.highlightColor)
            description: qsTr("%n track(s)", String(), model.tracksCount)
            mediaArt: {
                if (unknownArtist || unknownAlbum)
                    return String()
                return Unplayer.Utils.mediaArt(artist, model.album)
            }

            onClicked: pageStack.push("AlbumPage.qml", {
                                          allArtists: false,
                                          unknownAlbum: albumDelegate.unknownAlbum,
                                          album: model.album,
                                          unknownArtist: artistPage.unknownArtist,
                                          artist: artistPage.artist,
                                          tracksCount: model.tracksCount,
                                          duration: model.duration
                                      })
        }
        model: Unplayer.FilterProxyModel {
            filterRoleName: "album"
            sourceModel: AlbumsModel {
                allArtists: false
                unknownArtist: artistPage.unknownArtist
                artist: artistPage.artist
            }
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("All tracks")
                onClicked: pageStack.push("AllTracksPage.qml", {
                                              title: artistPage.artist,
                                              unknownArtist: artistPage.unknownArtist,
                                              artist: artistPage.artist
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
