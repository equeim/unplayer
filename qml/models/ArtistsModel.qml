import QtSparql 1.0

SparqlListModel {
    connection: SparqlConnection {
        driver: "QTRACKER_DIRECT"
    }
    query: "SELECT tracker:coalesce(nmm:artistName(nmm:performer(?track)), \"" + qsTr("Unknown artist") + "\") AS ?artist\n" +
           "       nmm:artistName(nmm:performer(?track)) AS ?rawArtist\n" +
           "       COUNT(DISTINCT(tracker:coalesce(nmm:musicAlbum(?track), 0))) AS ?albumsCount\n" +
           "       COUNT(?track) AS ?tracksCount\n" +
           "       SUM(nfo:duration(?track)) AS ?duration\n" +
           "WHERE {\n" +
           "    ?track a nmm:MusicPiece.\n" +
           "}\n" +
           "GROUP BY ?rawArtist\n" +
           "ORDER BY !bound(?rawArtist) ?rawArtist"
}
