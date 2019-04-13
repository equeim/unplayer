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

Column {
    property bool center

    anchors {
        left: parent.left
        leftMargin: Theme.horizontalPageMargin
        right: parent.right
        rightMargin: Theme.horizontalPageMargin
    }

    Label {
        anchors {
            left: parent.left
            right: parent.right
        }
        color: Theme.highlightColor
        horizontalAlignment: center && implicitWidth < width ? Text.AlignHCenter : Text.AlignLeft
        truncationMode: TruncationMode.Fade
        text: {
            switch (Unplayer.LibraryUtils.updateStage) {
            case Unplayer.LibraryUtils.PreparingStage:
                return qsTranslate("unplayer", "Preparing")
            case Unplayer.LibraryUtils.ScanningStage:
                return qsTranslate("unplayer", "Scanning filesystem")
            case Unplayer.LibraryUtils.ExtractingStage:
                return qsTranslate("unplayer", "Extracting metadata")
            case Unplayer.LibraryUtils.FinishingStage:
                return qsTranslate("unplayer", "Finishing")
            default:
                return ""
            }
        }
    }

    Label {
        anchors {
            left: parent.left
            right: parent.right
        }
        visible: switch (Unplayer.LibraryUtils.updateStage) {
                     case Unplayer.LibraryUtils.ScanningStage:
                     case Unplayer.LibraryUtils.ExtractingStage:
                         return true
                     default:
                         return false
                 }

        color: Theme.highlightColor
        horizontalAlignment: center && implicitWidth < width ? Text.AlignHCenter : Text.AlignLeft
        truncationMode: TruncationMode.Fade
        text: {
            switch (Unplayer.LibraryUtils.updateStage) {
            case Unplayer.LibraryUtils.ScanningStage:
                return qsTranslate("unplayer", "%n new tracks found", "", Unplayer.LibraryUtils.foundTracks)
            case Unplayer.LibraryUtils.ExtractingStage:
                return qsTranslate("unplayer", "%1 of %n tracks processed", "", Unplayer.LibraryUtils.foundTracks).arg(Unplayer.LibraryUtils.extractedTracks)
            default:
                return ""
            }
        }
    }
}
