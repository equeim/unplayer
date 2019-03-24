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
import org.nemomobile.dbus 2.0

import harbour.unplayer 0.1 as Unplayer

import "components"

ApplicationWindow
{
    id: rootWindow

    property bool largeScreen: Screen.sizeCategory === Screen.Large ||
                               Screen.sizeCategory === Screen.ExtraLarge

    signal mediaArtReloadNeeded()

    //property alias modelDialog: modelDialog

    _defaultPageOrientations: Orientation.All
    allowedOrientations: defaultAllowedOrientations
    bottomMargin: nowPlayingPanel.visibleSize
    cover: Qt.resolvedUrl("components/Cover.qml")
    initialPage: Qt.resolvedUrl("components/MainPage.qml")

    Component.onCompleted: {
        if (Unplayer.LibraryUtils.databaseInitialized) {
            if (Unplayer.LibraryUtils.createdTable && Unplayer.Settings.hasLibraryDirectories) {
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

    Rectangle {
        id: modalDialog

        property bool active: false
        property string text

        anchors.fill: parent
        color: Theme.rgba("black", 0.8)
        opacity: active ? 1 : 0
        Behavior on opacity { FadeAnimator { } }

        Binding on active {
            //when: Unplayer.LibraryUtils.updating
            value: Unplayer.LibraryUtils.updating
        }

        Binding on text {
            when: Unplayer.LibraryUtils.updating
            value: qsTranslate("unplayer", "Updating library...")
        }

        SilicaFlickable {
            anchors.fill: parent
            enabled: modalDialog.active

            BusyIndicator {
                id: busyIndicator

                anchors {
                    bottom: placeholder.top
                    bottomMargin: Theme.paddingLarge
                    horizontalCenter: parent.horizontalCenter
                }
                size: BusyIndicatorSize.Large
                running: modalDialog.active
            }

            ViewPlaceholder {
                id: placeholder
                verticalOffset: (busyIndicator.height + Theme.paddingLarge) / 2
                enabled: modalDialog.active
                text: modalDialog.text
            }
        }
    }
}
