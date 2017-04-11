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
                text: qsTranslate("unplayer", "Prefer cover art located as separate file in music file directory instead of extracted from music file")
                onCheckedChanged: Unplayer.Settings.useDirectoryMediaArt = checked
                Component.onCompleted: checked = Unplayer.Settings.useDirectoryMediaArt
            }

            SectionHeader {
                text: qsTranslate("unplayer", "Library")
            }

            TextSwitch {
                text: qsTranslate("unplayer", "Open library on startup")
                onCheckedChanged: Unplayer.Settings.openLibraryOnStartup = checked
                Component.onCompleted: checked = Unplayer.Settings.openLibraryOnStartup
            }

            BackgroundItem {
                id: libraryDirectoriesItem

                onClicked: pageStack.push("LibraryDirectoriesPage.qml")

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
            }
        }

        VerticalScrollDecorator { }
    }
}
