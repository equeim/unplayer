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

Page {
    SilicaFlickable {
        anchors.fill: parent

        Column {
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTranslate("unplayer", "Donate")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Theme.buttonWidthLarge
                text: "PayPal"
                onClicked: Qt.openUrlExternally("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=DDQTRHTY5YV2G&item_name=Support%20Unplayer%20development&no_note=1&item_number=1&no_shipping=1&currency_code=EUR")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Theme.buttonWidthLarge
                text: "Yandex.Money"
                onClicked: Qt.openUrlExternally("https://yasobe.ru/na/unplayer")
            }
        }
    }
}
