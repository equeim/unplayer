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

QtObject {
    id: bindings

    property bool when

    Component.onDestruction: {
        busyPanelActiveBinding.when = false
        busyPanelTextBinding.when = false
    }

    property Binding busyPanelActiveBinding: Binding {
        target: nowPlayingPanel
        property: "busyPanelActive"
        value: true
        when: bindings.when
    }

    property Binding busyPanelTextBinding: Binding {
        target: nowPlayingPanel
        property: "busyPanelText"
        value: qsTranslate("unplayer", "Removing files...")
        when: bindings.when
    }
}
