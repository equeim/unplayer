/*
 * Unplayer
 * Copyright (C) 2015-2020 Alexey Rochev <equeim@gmail.com>
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

import QtQuick 2.6
import Sailfish.Silica 1.0

SilicaListView {
    id: listView

    property Page page
    property string emptyText

    property int originalCacheBuffer
    property var sourceModel: model.sourceModel ? model.sourceModel : model

    //
    // If model is loaded before Page's push animation is completed,
    // delegates creation causes visible frame drop.
    // Set cacheBuffer to 0 until animation is complete to create only
    // visible delegates and reduce frame drop
    //

    Component.onCompleted: {
        originalCacheBuffer = cacheBuffer
        cacheBuffer = 0
    }

    Connections {
        target: page
        onStatusChanged: {
            if (status === PageStatus.Active) {
                if (listView.cacheBuffer !== listView.originalCacheBuffer) {
                    listView.cacheBuffer = listView.originalCacheBuffer
                }
            }
        }
    }

    ListViewPlaceholder {
        id: placeholder
        listView: listView
        loading: sourceModel.loading
        text: emptyText
    }

    BusyIndicator {
        anchors.horizontalCenter: parent.horizontalCenter
        y: Math.round(page.isPortrait ? Screen.height/4 : Screen.width/4) + placeholder.verticalOffset
        size: BusyIndicatorSize.Large
        running: sourceModel.loading
    }

    VerticalScrollDecorator { }
}
