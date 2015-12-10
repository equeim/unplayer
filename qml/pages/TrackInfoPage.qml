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

Page {
    property string trackUrl
    property var track: sparqlConnection.select("SELECT tracker:coalesce(nie:title(?track), nfo:fileName(?track)) AS ?title\n" +
                                                "       nmm:artistName(nmm:performer(?track)) AS ?artist\n" +
                                                "       nie:title(nmm:musicAlbum(?track)) AS ?album\n" +
                                                "       nmm:trackNumber(?track) AS ?trackNumber\n" +
                                                "       nfo:genre(?track) AS ?genre\n" +
                                                "       nfo:fileSize(?track) AS ?fileSize\n" +
                                                "       nie:mimeType(?track) AS ?mimeType\n" +
                                                "       nfo:duration(?track) AS ?duration\n" +
                                                "       nfo:averageBitrate(?track) AS ?averageBitrate\n" +
                                                "WHERE {\n" +
                                                "    ?track nie:url \"" + trackUrl + "\".\n" +
                                                "}")[0]

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTr("Track information")
            }

            DetailItem {
                label: qsTr("Title")
                value: track.title
            }

            DetailItem {
                label: qsTr("Artist")
                value: track.artist ? track.artist :
                                      qsTr("Unknown artist")
            }

            DetailItem {
                label: qsTr("Album")
                value: track.album ? track.album :
                                     qsTr("Unknown album")
            }

            DetailItem {
                label: qsTr("Track number")
                value: track.trackNumber ? track.trackNumber : String()
                visible: track.trackNumber !== undefined
            }

            DetailItem {
                label: qsTr("Genre")
                value: track.genre ? track.genre : String()
                visible: track.genre !== undefined
            }

            DetailItem {
                label: qsTr("File path")
                value: Unplayer.Utils.urlToPath(trackUrl)
            }

            DetailItem {
                label: qsTr("File size")
                value: Unplayer.Utils.formatByteSize(track.fileSize)
            }

            DetailItem {
                label: qsTr("MIME type")
                value: track.mimeType
            }

            DetailItem {
                visible: track.duration !== undefined
                label: qsTr("Duration")
                value: track.duration ? Format.formatDuration(track.duration, track.duration >= 3600 ? Format.DurationLong :
                                                                                                       Format.DurationShort) :
                                        String()
            }

            DetailItem {
                label: qsTr("Bitrate")
                value: track.averageBitrate ? track.averageBitrate : String()
                visible: track.averageBitrate !== undefined
            }
        }

        VerticalScrollDecorator { }
    }
}
