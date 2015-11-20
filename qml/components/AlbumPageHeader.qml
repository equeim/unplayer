import QtQuick 2.2
import Sailfish.Silica 1.0

import harbour.unplayer 0.1 as Unplayer

Item {
    property alias searchFieldY: pageHeader.height

    width: listView.width
    height: listView.showSearchField ? pageHeader.height + listView.searchFieldHeight :
                                       childrenRect.height

    Behavior on height {
        PropertyAnimation { }
    }

    PageHeader {
        id: pageHeader
        title: album
    }

    Rectangle {
        anchors {
            left: parent.left
            right: parent.right
            top: pageHeader.bottom
        }
        height: Theme.itemSizeExtraLarge + Theme.paddingLarge * 2

        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: "transparent"
            }
            GradientStop {
                position: 1.0
                color: Theme.rgba(Theme.highlightBackgroundColor, 0.2)
            }
        }

        opacity: listView.showSearchField ? 0 : 1

        Behavior on opacity {
            FadeAnimation { }
        }

        MediaArt {
            id: mediaArt

            anchors.verticalCenter: parent.verticalCenter
            x: Theme.horizontalPageMargin

            source: {
                if (unknownAlbum || unknownArtist)
                    return String()
                return Unplayer.Utils.mediaArt(artist, album)
            }

            size: Theme.itemSizeExtraLarge
        }

        Column {
            anchors {
                left: mediaArt.right
                leftMargin: Theme.paddingLarge
                right: parent.right
                rightMargin: Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }

            Label {
                font.pixelSize: Theme.fontSizeSmall
                text: artist
                textFormat: Text.StyledText
                width: parent.width
                truncationMode: TruncationMode.Fade
            }

            Label {
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("%n track(s)", String(), tracksCount)
                width: parent.width
                truncationMode: TruncationMode.Fade
            }

            Label {
                font.pixelSize: Theme.fontSizeSmall
                text: Unplayer.Utils.formatDuration(duration)
                width: parent.width
                truncationMode: TruncationMode.Fade
            }
        }
    }
}
