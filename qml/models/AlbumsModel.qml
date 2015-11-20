import QtSparql 1.0

SparqlListModel {
    property bool allArtists
    property bool unknownArtist
    property string artist

    connection: SparqlConnection {
        driver: "QTRACKER_DIRECT"
    }
    query: {
        var queryString =
                "SELECT ?album ?rawAlbum ?artist ?rawArtist ?tracksCount ?duration\n" +
                "WHERE {\n" +
                "    {\n" +
                "        SELECT ?track\n" +
                "               tracker:coalesce(nie:title(nmm:musicAlbum(?track)), \"" + qsTr("Unknown album") + "\") AS ?album\n" +
                "               nie:title(nmm:musicAlbum(?track)) AS ?rawAlbum\n" +
                "               tracker:coalesce(nmm:artistName(nmm:performer(?track)), \"" + qsTr("Unknown artist") + "\") AS ?artist\n" +
                "               nmm:artistName(nmm:performer(?track)) AS ?rawArtist\n" +
                "               nie:informationElementDate(?track) AS ?year\n" +
                "               COUNT(?track) AS ?tracksCount\n" +
                "               SUM(nfo:duration(?track)) AS ?duration\n" +
                "        WHERE {\n" +
                "            ?track a nmm:MusicPiece.\n" +
                "        }\n" +
                "        GROUP BY ?rawAlbum ?rawArtist\n" +
                "        ORDER BY !bound(?rawArtist) ?rawArtist !bound(?rawAlbum) ?year\n" +
                "    }.\n"

        if (allArtists) {
            queryString += "}"
            return queryString
        }

        if (unknownArtist)
            queryString += "    FILTER(!bound(?rawArtist)).\n"
        else
            queryString += "    FILTER(?rawArtist = \"" + artist + "\").\n"

        queryString += "}"

        return queryString
    }
}
