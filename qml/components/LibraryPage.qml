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
    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {
            MenuItem {
                text: qsTranslate("unplayer", "Reset Library")
                onClicked: Unplayer.LibraryUtils.resetDatabase()
            }

            MenuItem {
                text: qsTranslate("unplayer", "Update Library")
                onClicked: Unplayer.LibraryUtils.updateDatabase()
            }
        }

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTranslate("unplayer", "Library")
            }

            MainPageListItem {
                title: qsTranslate("unplayer", "Artists")
                description: qsTranslate("unplayer", "%n artist(s)", String(), Unplayer.LibraryUtils.artistsCount)
                randomMediaArt: Unplayer.RandomMediaArt {}
                onClicked: pageStack.push(artistsPageComponent)

                Component {
                    id: artistsPageComponent
                    ArtistsPage { }
                }
            }

            MainPageListItem {
                title: qsTranslate("unplayer", "Albums")
                description: qsTranslate("unplayer", "%n albums(s)", String(), Unplayer.LibraryUtils.albumsCount)
                randomMediaArt: Unplayer.RandomMediaArt {}
                onClicked: pageStack.push(allAlbumsPageComponent)

                Component {
                    id: allAlbumsPageComponent
                    AllAlbumsPage { }
                }
            }

            MainPageListItem {
                title: qsTranslate("unplayer", "Tracks")
                description: {
                    var tracksCount = Unplayer.LibraryUtils.tracksCount
                    if (!tracksCount === 0) {
                        return qsTranslate("unplayer", "%n tracks(s)", String(), 0)
                    }
                    return qsTranslate("unplayer", "%1, %2")
                    .arg(qsTranslate("unplayer", "%n tracks(s)", String(), tracksCount))
                    .arg(Unplayer.Utils.formatDuration(Unplayer.LibraryUtils.tracksDuration))
                }
                randomMediaArt: Unplayer.RandomMediaArt {}

                onClicked: pageStack.push(tracksPageComponent)

                Component {
                    id: tracksPageComponent
                    TracksPage {
                        pageTitle: qsTranslate("unplayer", "Tracks")
                        queryMode: Unplayer.TracksModel.QueryAllTracks
                    }
                }
            }

            MainPageListItem {
                title: qsTranslate("unplayer", "Genres")
                randomMediaArt: Unplayer.RandomMediaArt {}
                onClicked: pageStack.push("GenresPage.qml")
            }
        }

        VerticalScrollDecorator { }
    }
}
