import harbour.unplayer 0.1 as Unplayer

Unplayer.FilterProxyModel {
    function get(trackIndex) {
        return sourceModel.get(sourceIndex(trackIndex))
    }

    function getTracks() {
        var tracks = []
        var count = count()
        for (var i = 0; i < count; i++)
            tracks.push(sourceModel.get(sourceIndex(i)))
        return tracks
    }

    filterRoleName: "title"
}
