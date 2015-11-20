import QtQuick 2.2
import Sailfish.Silica 1.0

import harbour.unplayer 0.1 as Unplayer

Dialog {
    property var tracks

    canAccept: playlistNameField.text

    onAccepted: Unplayer.Utils.newPlaylist(playlistNameField.text, tracks)

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            DialogHeader {
                title: qsTr("Add playlist")
            }

            TextField {
                id: playlistNameField
                label: qsTr("Playlist name")
                placeholderText: label
                width: parent.width

                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: accept()

                Component.onCompleted: forceActiveFocus()
            }
        }
    }
}
