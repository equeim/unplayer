import Sailfish.Silica 1.0

import harbour.unplayer 0.1 as Unplayer

import "../components"
import "../models"

Page {
    id: page

    property string title

    property alias allArtists: tracksModel.allArtists

    property alias unknownArtist: tracksModel.unknownArtist
    property alias artist: tracksModel.artist

    SearchListView {
        id: listView

        anchors.fill: parent
        headerTitle: title
        delegate: LibraryTrackDelegate {
            allAlbums: true
        }
        model: TracksProxyModel {
            id: tracksProxyModel

            sourceModel: TracksModel {
                id: tracksModel
                allAlbums: true
            }
        }
        section {
            property: "artist"
            delegate: SectionHeader {
                text: section
            }
        }

        LibraryTracksPullDownMenu { }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No tracks")
        }
    }
}
