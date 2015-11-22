import Sailfish.Silica 1.0

import harbour.unplayer 0.1 as Unplayer

MediaContainerListItem {
    id: albumDelegate

    property bool unknownAlbum: model.rawAlbum === undefined
    property bool unknownArtist: model.rawArtist === undefined

    function getTracks() {
        return sparqlConnection.select(Unplayer.Utils.tracksSparqlQuery(false,
                                                                        false,
                                                                        model.artist,
                                                                        unknownArtist,
                                                                        model.album,
                                                                        unknownAlbum))
    }

    title: Theme.highlightText(model.album, listView.searchFieldText.trim(), Theme.highlightColor)
    mediaArt: {
        if (unknownArtist || unknownAlbum)
            return String()
        return Unplayer.Utils.mediaArt(model.artist, model.album)
    }
    menu: ContextMenu {
        MenuItem {
            text: qsTr("Add to queue")
            onClicked: {
                player.queue.add(getTracks())
                if (player.queue.currentIndex === -1) {
                    player.queue.currentIndex = 0
                    player.queue.currentTrackChanged()
                }
            }
        }

        MenuItem {
            text: qsTr("Add to playlist")
            onClicked: pageStack.push("../pages/AddToPlaylistPage.qml", { tracks: getTracks() })
        }
    }

    onClicked: pageStack.push("../pages/AlbumPage.qml", {
                                  album: model.album,
                                  unknownAlbum: albumDelegate.unknownAlbum,
                                  artist: model.artist,
                                  unknownArtist: albumDelegate.unknownArtist,
                                  tracksCount: model.tracksCount,
                                  duration: model.duration
                              })
}
