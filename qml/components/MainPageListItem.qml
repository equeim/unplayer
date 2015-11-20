import QtQuick 2.2
import Sailfish.Silica 1.0

ListItem {
    id: listItem

    property alias title: titleLabel.text
    property alias description: descriptionLabel.text
    property alias mediaArt: mediaArt.source

    contentHeight: Theme.itemSizeExtraLarge

    MediaArt {
        id: mediaArt
        highlighted: listItem.highlighted
        size: contentHeight
        x: Theme.horizontalPageMargin
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
            id: titleLabel
            color: highlighted ? Theme.highlightColor : Theme.primaryColor
            font.pixelSize: Theme.fontSizeLarge
            textFormat: Text.StyledText
            truncationMode: TruncationMode.Fade
            width: parent.width
        }

        Label {
            id: descriptionLabel
            color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
            font.pixelSize: Theme.fontSizeSmall
            truncationMode: TruncationMode.Fade
            width: parent.width
        }
    }
}
