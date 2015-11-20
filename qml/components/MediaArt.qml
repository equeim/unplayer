import QtQuick 2.2
import Sailfish.Silica 1.0

Item {
    property bool highlighted
    property alias source: mediaArtImage.source
    property int size

    width: size
    height: size

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: Theme.rgba(Theme.primaryColor, 0.1)
            }
            GradientStop {
                position: 1.0
                color: Theme.rgba(Theme.primaryColor, 0.05)
            }
        }
        visible: !mediaArtImage.visible

        Image {
            anchors.centerIn: parent
            asynchronous: true
            source: highlighted ? "image://theme/icon-m-music?" + Theme.highlightColor :
                                  "image://theme/icon-m-music"
            visible: mediaArtImage.status != Image.Loading
        }
    }

    Image {
        id: mediaArtImage
        anchors.fill: parent
        asynchronous: true
        fillMode: Image.PreserveAspectCrop
        sourceSize.height: size
        visible: status === Image.Ready

        Rectangle {
            anchors.fill: parent
            color: Theme.highlightBackgroundColor
            opacity: Theme.highlightBackgroundOpacity
            visible: highlighted
        }
    }
}
