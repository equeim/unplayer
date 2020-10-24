/*
 * Unplayer
 * Copyright (C) 2015-2019 Alexey Rochev <equeim@gmail.com>
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
    property bool libraryChanged

    Component.onDestruction: {
        if (libraryChanged) {
            Unplayer.LibraryUtils.updateDatabase()
        } else if (mediaArtSwitch.checked !== mediaArtSwitch.useDirectoryMediaArt) {
            Unplayer.LibraryUtils.mediaArtChanged()
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTranslate("unplayer", "Settings")
            }

            SectionHeader {
                text: qsTranslate("unplayer", "Player")
            }

            TextSwitch {
                text: qsTranslate("unplayer", "Restore player state on startup")
                onCheckedChanged: Unplayer.Settings.restorePlayerState = checked
                Component.onCompleted: checked = Unplayer.Settings.restorePlayerState
            }

            TextSwitch {
                id: mediaArtSwitch

                property bool useDirectoryMediaArt

                checked: useDirectoryMediaArt
                text: qsTranslate("unplayer", "Prefer cover art located as separate file in music file directory instead of extracted from music file")

                onCheckedChanged: Unplayer.Settings.useDirectoryMediaArt = checked
                Component.onCompleted: useDirectoryMediaArt = Unplayer.Settings.useDirectoryMediaArt
            }

            TextSwitch {
                text: qsTranslate("unplayer", "Show audio codec information on Now Playing screen")
                checked: Unplayer.Settings.showNowPlayingCodecInfo
                onCheckedChanged: Unplayer.Settings.showNowPlayingCodecInfo = checked
            }

            SectionHeader {
                text: qsTranslate("unplayer", "Directories")
            }

            TextSwitch {
                text: qsTranslate("unplayer", "Show video files")
                onCheckedChanged: Unplayer.Settings.showVideoFiles = checked
                Component.onCompleted: checked = Unplayer.Settings.showVideoFiles
            }

            SectionHeader {
                text: qsTranslate("unplayer", "Library")
            }

            TextSwitch {
                text: qsTranslate("unplayer", "Open library on startup")
                onCheckedChanged: Unplayer.Settings.openLibraryOnStartup = checked
                Component.onCompleted: checked = Unplayer.Settings.openLibraryOnStartup
            }

            TextSwitch {
                text: qsTranslate("unplayer", "Use tag \"Album artist\" instead of \"Artist\" if it is available")
                onCheckedChanged: Unplayer.Settings.useAlbumArtist = checked
                Component.onCompleted: checked = Unplayer.Settings.useAlbumArtist
            }

            BackgroundItem {
                id: libraryDirectoriesItem

                onClicked: pageStack.push(libraryDirectoriesPageComponent)

                Label {
                    anchors {
                        left: parent.left
                        leftMargin: Theme.horizontalPageMargin
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    text: qsTranslate("unplayer", "Library Directories")
                    color: libraryDirectoriesItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                    truncationMode: TruncationMode.Fade
                }

                Component {
                    id: libraryDirectoriesPageComponent
                    LibraryDirectoriesPage {
                        title: qsTranslate("unplayer", "Library Directories")
                        model: Unplayer.LibraryDirectoriesModel {
                            type: Unplayer.LibraryDirectoriesModel.Library
                        }
                        Component.onDestruction: libraryChanged = changed
                    }
                }
            }

            BackgroundItem {
                id: blacklistedDirectoriesItem

                onClicked: pageStack.push(blacklistedDirectoriesPageComponent)

                Label {
                    anchors {
                        left: parent.left
                        leftMargin: Theme.horizontalPageMargin
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    text: qsTranslate("unplayer", "Blacklisted Directories")
                    color: blacklistedDirectoriesItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                    truncationMode: TruncationMode.Fade
                }

                Component {
                    id: blacklistedDirectoriesPageComponent
                    LibraryDirectoriesPage {
                        title: qsTranslate("unplayer", "Blacklisted Directories")
                        model: Unplayer.LibraryDirectoriesModel {
                            type: Unplayer.LibraryDirectoriesModel.Blacklisted
                        }
                        Component.onDestruction: libraryChanged = changed
                    }
                }
            }
        }

        VerticalScrollDecorator { }
    }
}
