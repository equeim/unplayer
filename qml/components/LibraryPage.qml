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
    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {
            MenuItem {
                text: qsTr("Reset Library")
                onClicked: Unplayer.LibraryUtils.resetDatabase()
            }

            MenuItem {
                text: qsTr("Update Library")
                onClicked: Unplayer.LibraryUtils.updateDatabase()
            }
        }

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTr("Library")
            }

            MainPageListItem {
                title: qsTr("Artists")
                description: qsTr("%n artist(s)", String(), Unplayer.LibraryUtils.artistsCount)
                mediaArt: Unplayer.LibraryUtils.randomMediaArt
                onClicked: pageStack.push(artistsPageComponent)

                Component {
                    id: artistsPageComponent
                    ArtistsPage { }
                }
            }

            MainPageListItem {
                title: qsTr("Albums")
                description: qsTr("%n albums(s)", String(), Unplayer.LibraryUtils.albumsCount)
                mediaArt: Unplayer.LibraryUtils.randomMediaArt
                onClicked: pageStack.push(allAlbumsPageComponent)

                Component {
                    id: allAlbumsPageComponent
                    AllAlbumsPage { }
                }
            }

            MainPageListItem {
                title: qsTr("Tracks")
                description: {
                    var tracksCount = Unplayer.LibraryUtils.tracksCount
                    if (tracksCount === 0) {
                        return qsTr("%n tracks(s)", String(), 0)
                    }
                    return qsTr("%1, %2")
                    .arg(qsTr("%n tracks(s)", String(), tracksCount))
                    .arg(Unplayer.Utils.formatDuration(Unplayer.LibraryUtils.tracksDuration))
                }
                mediaArt: Unplayer.LibraryUtils.randomMediaArt

                onClicked: pageStack.push(tracksPageComponent)

                Component {
                    id: tracksPageComponent
                    TracksPage {
                        pageTitle: qsTr("Tracks")
                        allArtists: true
                    }
                }
            }

            MainPageListItem {
                title: qsTr("Genres")
                onClicked: pageStack.push("GenresPage.qml")
            }
        }

        VerticalScrollDecorator { }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.rgba("black", 0.8)
        opacity: Unplayer.LibraryUtils.updating ? 1 : 0
        Behavior on opacity { FadeAnimation { } }

        SilicaFlickable {
            anchors.fill: parent
            enabled: Unplayer.LibraryUtils.updating

            BusyIndicator {
                id: busyIndicator

                anchors {
                    bottom: updatingPlaceholder.top
                    bottomMargin: Theme.paddingLarge
                    horizontalCenter: parent.horizontalCenter
                }
                size: BusyIndicatorSize.Large
                running: Unplayer.LibraryUtils.updating
            }

            ViewPlaceholder {
                id: updatingPlaceholder
                verticalOffset: (busyIndicator.height + Theme.paddingLarge) / 2
                enabled: Unplayer.LibraryUtils.updating
                text: qsTr("Updating library...")
            }

            /*MouseArea {
                anchors.fill: parent
                enabled: Unplayer.LibraryUtils.updating
            }*/
        }
    }
}
