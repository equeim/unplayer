import QtQuick 2.2
import Sailfish.Silica 1.0

import harbour.unplayer 0.1 as Unplayer

SilicaListView {
    id: listView

    property bool showSearchField: false
    property alias searchFieldText: searchField.text
    property alias searchFieldHeight: searchField.height

    property string headerTitle

    header: Item {
        property int searchFieldY: childrenRect.height

        width: listView.width
        height: showSearchField ? childrenRect.height + searchFieldHeight :
                                  childrenRect.height

        Behavior on height {
            PropertyAnimation { }
        }

        PageHeader {
            title: headerTitle
        }
    }

    onShowSearchFieldChanged: {
        if (showSearchField)
            searchField.forceActiveFocus()
    }

    SearchField {
        id: searchField

        parent: listView.parent

        width: parent.width
        y: -contentY - headerItem.height + headerItem.searchFieldY
        z: 1

        enabled: showSearchField ? true : false
        opacity: showSearchField ? 1 : 0

        onActiveFocusChanged: {
            if (!activeFocus && text.length === 0)
                showSearchField = false
        }

        onTextChanged: {
            var regExpText = Unplayer.Utils.escapeRegExp(text.trim())
            if (regExpText.toLowerCase() === regExpText)
                model.filterRegExp = new RegExp(regExpText, "i")
            else
                model.filterRegExp = new RegExp(regExpText)
        }

        EnterKey.iconSource: "image://theme/icon-m-enter-close"
        EnterKey.onClicked: focus = false

        Behavior on opacity {
            FadeAnimation { }
        }
    }

    VerticalScrollDecorator { }
}
