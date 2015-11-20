import Sailfish.Silica 1.0

import harbour.unplayer 0.1 as Unplayer

import "../components"
import "../models"

Page {
    SearchListView {
        id: listView

        anchors.fill: parent
        headerTitle: qsTr("Artists")
        delegate: MediaContainerListItem {
            id: artistDelegate

            property bool unknownArtist: model.rawArtist === undefined

            title: Theme.highlightText(model.artist, listView.searchFieldText.trim(), Theme.highlightColor)
            description: qsTr("%n album(s)", String(), model.albumsCount)
            mediaArt: {
                if (unknownArtist)
                    return String()
                return Unplayer.Utils.mediaArtForArtist(model.artist);
            }

            onClicked: pageStack.push("ArtistPage.qml", {
                                          unknownArtist: artistDelegate.unknownArtist,
                                          artist: model.artist,
                                          albumsCount: model.albumsCount,
                                          tracksCount: model.tracksCount,
                                          duration: model.duration
                                      })
        }
        model: Unplayer.FilterProxyModel {
            filterRoleName: "artist"
            sourceModel: ArtistsModel { }
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
            text: qsTr("No artists")
        }
    }
}
