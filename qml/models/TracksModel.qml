import QtSparql 1.0

import harbour.unplayer 0.1 as Unplayer

SparqlListModel {
    property bool allArtists
    property bool allAlbums

    property string artist
    property bool unknownArtist

    property string album
    property bool unknownAlbum

    connection: SparqlConnection {
        driver: "QTRACKER_DIRECT"
    }
    query: Unplayer.Utils.tracksSparqlQuery(allArtists,
                                            allAlbums,
                                            artist,
                                            unknownArtist,
                                            album,
                                            unknownAlbum)
}
