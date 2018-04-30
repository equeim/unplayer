/*
 * Unplayer
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
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
                text: qsTranslate("unplayer", "About")
                onClicked: pageStack.push("AboutPage.qml")
            }

            MenuItem {
                text: qsTranslate("unplayer", "Settings")
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
                title: qsTranslate("unplayer", "Library")
                description: {
                    if (!Unplayer.LibraryUtils.databaseInitialized) {
                        return qsTranslate("unplayer", "Error initializing database")
                    }
                    if (!Unplayer.Settings.hasLibraryDirectories) {
                        return qsTranslate("unplayer", "No selected directories")
                    }
                    if (Unplayer.LibraryUtils.updating) {
                        return qsTranslate("unplayer", "Updating...")
                    }
                    var tracksCount = Unplayer.LibraryUtils.tracksCount
                    if (tracksCount === 0) {
                        return qsTranslate("unplayer", "%n tracks(s)", String(), 0)
                    }
                    return qsTranslate("unplayer", "%1, %2")
                    .arg(qsTranslate("unplayer", "%n tracks(s)", String(), tracksCount))
                    .arg(Unplayer.Utils.formatDuration(Unplayer.LibraryUtils.tracksDuration))
                }
                mediaArt: Unplayer.LibraryUtils.randomMediaArt

                menu: Component {
                    ContextMenu {
                        MenuItem {
                            enabled: Unplayer.Settings.hasLibraryDirectories
                            text: qsTranslate("unplayer", "Update Library")
                            onClicked: Unplayer.LibraryUtils.updateDatabase()
                        }

                        MenuItem {
                            text: qsTranslate("unplayer", "Reset Library")
                            onClicked: Unplayer.LibraryUtils.resetDatabase()
                        }
                    }
                }
                showMenuOnPressAndHold: !Unplayer.LibraryUtils.updating

                onClicked: {
                    if (Unplayer.Settings.hasLibraryDirectories) {
                        pageStack.push(libraryPageComponent)
                    } else {
                        pageStack.push(libraryDirectoriesPageComponent)
                    }
                }

                Component {
                    id: libraryPageComponent
                    LibraryPage { }
                }

                Component {
                    id: libraryDirectoriesPageComponent

                    LibraryDirectoriesPage {
                        Component.onDestruction: {
                            if (changed) {
                                Unplayer.LibraryUtils.updateDatabase()
                            }
                        }
                    }
                }
            }

            MainPageListItem {
                title: qsTranslate("unplayer", "Playlists")
                description: qsTranslate("unplayer", "%n playlist(s)", String(), Unplayer.PlaylistUtils.playlistsCount)
                fallbackIcon: "image://theme/icon-m-document"
                onClicked: pageStack.push(playlistsPageComponent)

                Component {
                    id: playlistsPageComponent
                    PlaylistsPage { }
                }
            }

            MainPageListItem {
                title: qsTranslate("unplayer", "Directories")
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
