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
                                          tracks: [tracksModel.get(tracksProxyModel.sourceIndex(model.index))]
                                      })
        }
    }

    onClicked: {
        if (current) {
            if (!player.playing)
                player.play()
            return
        }

        player.queue.clear()
        player.queue.add(tracksProxyModel.getTracks())

        player.queue.currentIndex = model.index
        player.queue.currentTrackChanged()
    }
}
