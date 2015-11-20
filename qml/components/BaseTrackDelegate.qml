import QtQuick 2.2
import Sailfish.Silica 1.0

ListItem {
    property bool allAlbums: false
    property bool current

    Column {
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        Label {
            color: highlighted || current ? Theme.highlightColor : Theme.primaryColor
            text: Theme.highlightText(model.title, listView.searchFieldText.trim(), Theme.highlightColor)
            textFormat: Text.StyledText
            truncationMode: TruncationMode.Fade
            width: parent.width
        }

        Item {
            width: parent.width
            height: childrenRect.height

            Label {
                anchors {
                    left: parent.left
                    right: durationLabel.left
                    rightMargin: Theme.paddingMedium
                }
                color: highlighted || current ? Theme.secondaryHighlightColor : Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: allAlbums ? model.artist + " - " + model.album :
                                  model.artist
                textFormat: Text.StyledText
                truncationMode: TruncationMode.Fade
            }

            Label {
                id: durationLabel

                anchors.right: parent.right
                color: highlighted || current ? Theme.secondaryHighlightColor : Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: Format.formatDuration(model.duration, model.duration >= 3600? Format.DurationLong :
                                                                                    Format.DurationShort)
            }
        }
    }
}
