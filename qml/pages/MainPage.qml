/*
 * Unplayer
 * Copyright (C) 2015 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.2
import Sailfish.Silica 1.0

import harbour.unplayer 0.1 as Unplayer

import "../components"

Page {
    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {
            MenuItem {
                text: qsTr("About")
                onClicked: pageStack.push("AboutPage.qml")
            }
        }

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: "Unplayer"
            }

            MainPageListItem {
                property int artistsCount: {
                    if (sparqlConnection.ready)
                        return sparqlConnection.select("SELECT nmm:artistName(nmm:performer(?track)) AS ?artist\n" +
                                                       "WHERE {\n" +
                                                       "    ?track a nmm:MusicPiece.\n" +
                                                       "}\n" +
                                                       "GROUP BY ?artist").length
                    return 0
                }

                title: qsTr("Artists")
                description: qsTr("%n artist(s)", String(), artistsCount)
                mediaArt: Unplayer.Utils.randomMediaArt()

                onClicked: pageStack.push("ArtistsPage.qml")
            }

            MainPageListItem {
                property int albumsCount: {
                    if (sparqlConnection.ready)
                        return sparqlConnection.select("SELECT nie:title(nmm:musicAlbum(?track)) AS ?album\n" +
                                                       "       nmm:artistName(nmm:performer(?track)) AS ?artist\n" +
                                                       "WHERE {\n" +
                                                       "    ?track a nmm:MusicPiece.\n" +
                                                       "}\n" +
                                                       "GROUP BY ?album ?artist").length
                    return 0
                }

                title: qsTr("Albums")
                description: qsTr("%n albums(s)", String(), albumsCount)
                mediaArt: Unplayer.Utils.randomMediaArt()

                onClicked: pageStack.push("AllAlbumsPage.qml")
            }

            MainPageListItem {
                property int duration: {
                    if (sparqlConnection.ready)
                        return sparqlConnection.select("SELECT SUM(nfo:duration(?track)) AS ?duration\n" +
                                                       "WHERE {\n" +
                                                       "    ?track a nmm:MusicPiece.\n" +
                                                       "}")[0].duration
                    return 0
                }

                property int tracksCount: {
                    if (sparqlConnection.ready)
                        return sparqlConnection.select("SELECT ?track\n" +
                                                       "WHERE {\n" +
                                                       "    ?track a nmm:MusicPiece.\n" +
                                                       "}").length
                    return 0
                }

                title: qsTr("Tracks")
                description: qsTr("%n tracks(s)", String(), tracksCount) + ", " + Unplayer.Utils.formatDuration(duration)
                mediaArt: Unplayer.Utils.randomMediaArt()

                onClicked: pageStack.push("TracksPage.qml", {
                                              pageTitle: qsTr("Tracks"),
                                              allArtists: true
                                          })
            }

            MainPageListItem {
                title: qsTr("Genres")
                onClicked: pageStack.push("GenresPage.qml")
            }

            MainPageListItem {
                id: playlistsListItem

                property int playlistsCount: {
                    if (sparqlConnection.ready)
                        return getPlaylistsCount()
                    return 0
                }

                function getPlaylistsCount() {
                    return sparqlConnection.select("SELECT ?playlist\n" +
                                                   "WHERE {\n" +
                                                   "    ?playlist a nmm:Playlist;\n" +
                                                   "              nie:mimeType ?mimeType.\n" +
                                                   "    FILTER(?mimeType = \"audio/x-scpls\")" +
                                                   "}").length
                }

                title: qsTr("Playlists")
                description: qsTr("%n playlist(s)", String(), playlistsCount)
                fallbackIcon: "image://theme/icon-m-document"
                onClicked: pageStack.push("PlaylistsPage.qml")

                Connections {
                    target: Unplayer.PlaylistUtils
                    onPlaylistsChanged: playlistsListItem.playlistsCount = playlistsListItem.getPlaylistsCount()
                }
            }

            MainPageListItem {
                title: qsTr("Directories")
                fallbackIcon: "image://theme/icon-m-folder"
                onClicked: pageStack.push("DirectoriesPage.qml")
            }
        }

        VerticalScrollDecorator { }
    }
}
