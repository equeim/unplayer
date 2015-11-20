import Sailfish.Silica 1.0

PullDownMenu {
    MenuItem {
        function getTrackUrls() {
            var trackUrls = []
            var count = tracksProxyModel.count()
            for (var i = 0; i < count; i++)
                trackUrls[i] = tracksModel.get(tracksProxyModel.sourceIndex(i)).url
            return trackUrls
        }

        text: qsTr("Add to playlist")
        onClicked: pageStack.push("../pages/AddToPlaylistPage.qml", {
                                      parentPage: page,
                                      tracks: getTrackUrls()
                                  })
    }

    MenuItem {
        text: qsTr("Add to queue")
        onClicked: {
            var tracks = []
            var count = tracksProxyModel.count()
            for (var i = 0; i < count; i++)
                tracks[i] = tracksModel.get(tracksProxyModel.sourceIndex(i))

            player.queue.add(tracks)
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
