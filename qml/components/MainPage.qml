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
                text: qsTr("About")
                onClicked: pageStack.push("AboutPage.qml")
            }

            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push("SettingsPage.qml")
            }
        }

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: "Unplayer"
            }

            MainPageListItem {
                enabled: Unplayer.LibraryUtils.databaseInitialized
                title: qsTr("Library")
                description: {
                    if (!Unplayer.LibraryUtils.databaseInitialized) {
                        return qsTr("Error initializing database")
                    }
                    if (!Unplayer.Settings.hasLibraryDirectories) {
                        return qsTr("No selected directories")
                    }
                    if (Unplayer.LibraryUtils.updating) {
                        return qsTr("Updating...")
                    }
                    var tracksCount = Unplayer.LibraryUtils.tracksCount
                    if (tracksCount === 0) {
                        return qsTr("%n tracks(s)", String(), 0)
                    }
                    return qsTr("%1, %2")
                    .arg(qsTr("%n tracks(s)", String(), tracksCount))
                    .arg(Unplayer.Utils.formatDuration(Unplayer.LibraryUtils.tracksDuration))
                }
                mediaArt: Unplayer.LibraryUtils.randomMediaArt

                menu: Component {
                    ContextMenu {
                        MenuItem {
                            enabled: Unplayer.Settings.hasLibraryDirectories
                            text: qsTr("Update Library")
                            onClicked: Unplayer.LibraryUtils.updateDatabase()
                        }

                        MenuItem {
                            text: qsTr("Reset Library")
                            onClicked: Unplayer.LibraryUtils.resetDatabase()
                        }
                    }
                }
                showMenuOnPressAndHold: !Unplayer.LibraryUtils.updating

                onClicked: {
                    if (Unplayer.Settings.hasLibraryDirectories) {
                        pageStack.push(libraryPageComponent)
                    } else {
                        pageStack.push("LibraryDirectoriesPage.qml")
                    }
                }

                Component {
                    id: libraryPageComponent
                    LibraryPage { }
                }
            }

            MainPageListItem {
                title: qsTr("Playlists")
                description: qsTr("%n playlist(s)", String(), Unplayer.PlaylistUtils.playlistsCount)
                fallbackIcon: "image://theme/icon-m-document"
                onClicked: pageStack.push(playlistsPageComponent)

                Component {
                    id: playlistsPageComponent
                    PlaylistsPage { }
                }
            }

            MainPageListItem {
                title: qsTr("Directories")
                fallbackIcon: "image://theme/icon-m-folder"
                onClicked: pageStack.push(directoriesPageComponent)

                Component {
                    id: directoriesPageComponent
                    DirectoriesPage { }
                }
            }
        }

        VerticalScrollDecorator { }
    }
}
