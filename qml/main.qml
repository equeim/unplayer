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

import Sailfish.Silica 1.0
import QtSparql 1.0

import harbour.unplayer 0.1 as Unplayer

import "components"

ApplicationWindow
{
    id: rootWindow

    property bool largeScreen: Screen.sizeCategory === Screen.Large ||
                               Screen.sizeCategory === Screen.ExtraLarge

    signal mediaArtReloadNeeded()

    _defaultPageOrientations: Orientation.All
    allowedOrientations: defaultAllowedOrientations
    bottomMargin: nowPlayingPanel.visibleSize
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    initialPage: Qt.resolvedUrl("pages/MainPage.qml")

    Unplayer.Player {
        id: player
    }

    SparqlConnection {
        id: sparqlConnection
        property bool ready: status === SparqlConnection.Ready
        driver: "QTRACKER_DIRECT"
    }

    NowPlayingPanel {
        id: nowPlayingPanel
    }
}
