/*
 * Unplayer
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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
    property alias filePath: trackInfo.filePath

    Unplayer.TrackInfo {
        id: trackInfo
    }

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
                value: trackInfo.title
            }

            DetailItem {
                label: qsTr("Artist")
                value: trackInfo.artist
            }

            DetailItem {
                label: qsTr("Album")
                value: trackInfo.album
            }

            DetailItem {
                label: qsTr("Track number")
                value: trackInfo.trackNumber ? trackInfo.trackNumber : String()
            }

            DetailItem {
                label: qsTr("Genre")
                value: trackInfo.genre
            }

            DetailItem {
                label: qsTr("File path")
                value: filePath
            }

            DetailItem {
                label: qsTr("File size")
                value: Unplayer.Utils.formatByteSize(trackInfo.fileSize)
            }

            DetailItem {
                label: qsTr("MIME type")
                value: trackInfo.mimeType
            }

            DetailItem {
                label: qsTr("Duration")
                value: trackInfo.hasAudioProperties ? Format.formatDuration(trackInfo.duration, trackInfo.duration >= 3600 ? Format.DurationLong :
                                                                                                                             Format.DurationShort) :
                                                      String()
            }

            DetailItem {
                label: qsTr("Bitrate")
                value: trackInfo.hasAudioProperties ? trackInfo.bitrate : String()
            }
        }

        VerticalScrollDecorator { }
    }
}
