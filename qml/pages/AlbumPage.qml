import Sailfish.Silica 1.0

import harbour.unplayer 0.1 as Unplayer

import "../components"
import "../models"

Page {
    id: page

    property alias album: tracksModel.album
    property alias unknownAlbum: tracksModel.unknownAlbum

    property alias artist: tracksModel.artist
    property alias unknownArtist: tracksModel.unknownArtist

    property int tracksCount
    property int duration

    SearchListView {
        id: listView

        anchors.fill: parent
        header: AlbumPageHeader { }
        delegate: LibraryTrackDelegate {
            allAlbums: false
        }
        model: Unplayer.FilterProxyModel {
            id: tracksProxyModel
            filterRoleName: "title"
            sourceModel: TracksModel {
                id: tracksModel

                allArtists: false
                allAlbums: false
            }
        }

        LibraryTracksPullDownMenu { }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No tracks")
        }
    }
}
