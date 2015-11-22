import QtQuick 2.2
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

            function getTracks() {
                return sparqlConnection.select(Unplayer.Utils.tracksSparqlQuery(false,
                                                                                true,
                                                                                model.artist,
                                                                                unknownArtist,
                                                                                String(),
                                                                                false))
            }

            function addTracksToQueue() {
                player.queue.add(getTracks())
                if (player.queue.currentIndex === -1) {
                    player.queue.currentIndex = 0
                    player.queue.currentTrackChanged()
                }
            }

            title: Theme.highlightText(model.artist, listView.searchFieldText.trim(), Theme.highlightColor)
            description: qsTr("%n album(s)", String(), model.albumsCount)
            mediaArt: {
                if (unknownArtist)
                    return String()
                return Unplayer.Utils.mediaArtForArtist(model.artist);
            }
            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Add to queue")
                    onClicked: addTracksToQueue()
                }

                MenuItem {
                    text: qsTr("Add to playlist")
                    onClicked: pageStack.push("AddToPlaylistPage.qml", { tracks: getTracks() })
                }
            }

            onClicked: pageStack.push(artistPage)

            Component {
                id: artistPage
                ArtistPage { }
            }
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
