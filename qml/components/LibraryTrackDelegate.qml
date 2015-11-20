import Sailfish.Silica 1.0

BaseTrackDelegate {
    current: model.url === player.queue.currentUrl
    menu: ContextMenu {
        MenuItem {
            text: qsTr("Add to queue")
            onClicked: {
                player.queue.add([tracksModel.get(tracksProxyModel.sourceIndex(model.index))])
                if (player.queue.currentIndex === -1) {
                    player.queue.currentIndex = 0
                    player.queue.currentTrackChanged()
                }
            }
        }

        MenuItem {
            text: qsTr("Add to playlist")
            onClicked: pageStack.push("../pages/AddToPlaylistPage.qml", {
                                          parentPage: page,
                                          tracks: [model.url]
                                      })
        }
    }

    onClicked: {
        if (current) {
            if (!player.playing)
                player.play()
            return
        }

        var trackList = []
        var count = tracksProxyModel.count()
        for (var i = 0; i < count; i++) {
            trackList[i] = tracksModel.get(tracksProxyModel.sourceIndex(i))
        }

        player.queue.clear()
        player.queue.add(trackList)

        player.queue.currentIndex = model.index
        player.queue.currentTrackChanged()
    }
}
