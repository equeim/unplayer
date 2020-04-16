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
import Nemo.DBus 2.0

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
    bottomMargin: nowPlayingPanel.parent === contentItem ? 0 : nowPlayingPanel.visibleSize
    cover: Qt.resolvedUrl("components/Cover.qml")
    initialPage: Qt.resolvedUrl("components/MainPage.qml")

    Component.onCompleted: {
        // Bring panel's parent on top
        nowPlayingPanel.parent.z = 99

        if (Unplayer.LibraryUtils.databaseInitialized) {
            if (Unplayer.LibraryUtils.createdTables && Unplayer.Settings.hasLibraryDirectories) {
                Unplayer.LibraryUtils.updateDatabase()
            }
            if (Unplayer.Settings.openLibraryOnStartup && Unplayer.Settings.hasLibraryDirectories) {
                pageStack.push("components/LibraryPage.qml", null, PageStackAction.Immediate)
            }
        }
        if (commandLineArguments.length) {
            Unplayer.Player.queue.addTracksFromUrls(commandLineArguments, true)
        } else if (Unplayer.Settings.restorePlayerState) {
            Unplayer.Player.restoreState()
        }
    }

    Component.onDestruction: Unplayer.Player.saveState()

    MediaKeys { }

    NowPlayingPanel {
        id: nowPlayingPanel
    }

    DBusAdaptor {
        service: "org.equeim.unplayer"
        iface: "org.equeim.unplayer"
        path: "/org/equeim/unplayer"

        function addTracksToQueue(tracks) {
            if (tracks.length) {
                Unplayer.Player.queue.addTracksFromUrls(tracks, true)
            }
        }
    }
}
