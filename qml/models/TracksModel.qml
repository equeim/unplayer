import QtSparql 1.0

SparqlListModel {
    property bool allArtists
    property bool allAlbums

    property bool unknownArtist
    property string artist

    property bool unknownAlbum
    property string album

    connection: SparqlConnection {
        driver: "QTRACKER_DIRECT"
    }
    query: {
        var queryString =
                "SELECT ?title ?url ?duration ?artist ?rawArtist ?album ?rawAlbum\n" +
                "WHERE {\n" +
                "    {\n" +
                "        SELECT tracker:coalesce(nie:title(?track), nfo:fileName(?track)) AS ?title\n" +
                "               nie:url(?track) AS ?url\n" +
                "               nfo:duration(?track) AS ?duration\n" +
                "               nmm:trackNumber(?track) AS ?trackNumber\n" +
                "               tracker:coalesce(nmm:artistName(nmm:performer(?track)), \"" + qsTr("Unknown artist") + "\") AS ?artist\n" +
                "               nmm:artistName(nmm:performer(?track)) AS ?rawArtist\n" +
                "               tracker:coalesce(nie:title(nmm:musicAlbum(?track)), \"" + qsTr("Unknown album") + "\") AS ?album\n" +
                "               nie:title(nmm:musicAlbum(?track)) AS ?rawAlbum\n" +
                "               nie:informationElementDate(?track) AS ?year\n" +
                "        WHERE {\n" +
                "            ?track a nmm:MusicPiece.\n" +
                "        }\n" +
                "        ORDER BY !bound(?rawArtist) ?rawArtist !bound(?rawAlbum) ?year ?rawAlbum ?trackNumber ?title\n" +
                "    }.\n"

        if (!allArtists) {
            if (unknownArtist)
                queryString += "    FILTER(!bound(?rawArtist)).\n"
            else
                queryString += "    FILTER(?rawArtist = \"" + artist + "\").\n"
        }

        if (!allAlbums) {
            if (unknownAlbum)
                queryString += "    FILTER(!bound(?rawAlbum)).\n"
            else
                queryString += "    FILTER(?rawAlbum = \"" + album + "\").\n"
        }

        queryString += "}"

        return queryString
    }
}
