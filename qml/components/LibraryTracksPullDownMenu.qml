import Sailfish.Silica 1.0

PullDownMenu {
    MenuItem {
        text: qsTr("Add to playlist")
        onClicked: pageStack.push("../pages/AddToPlaylistPage.qml", {
                                      parentPage: page,
                                      tracks: tracksProxyModel.getTracks()
                                  })
    }

    MenuItem {
        text: qsTr("Add to queue")
        onClicked: {
            player.queue.add(tracksProxyModel.getTracks())
            if (player.queue.currentIndex === -1) {
                player.queue.currentIndex = 0
                player.queue.currentTrackChanged()
            }
        }
    }

    MenuItem {
        enabled: listView.count !== 0
        text: qsTr("Search")
        onClicked: listView.showSearchField = true
    }
}
