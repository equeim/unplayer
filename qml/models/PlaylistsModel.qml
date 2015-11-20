import QtSparql 1.0

SparqlListModel {
    connection: SparqlConnection {
        driver: "QTRACKER_DIRECT"
    }
    query: "SELECT tracker:coalesce(nie:title(?playlist), tracker:string-from-filename(nfo:fileName(?playlist))) AS ?title\n" +
           "       nie:url(?playlist) AS ?url\n" +
           "       nfo:entryCounter(?playlist) AS ?tracksCount\n" +
           "WHERE {\n" +
           "    ?playlist a nmm:Playlist;\n" +
           "              nie:mimeType ?mimeType.\n" +
           "    FILTER(?mimeType = \"audio/x-scpls\")" +
           "}\n" +
           "ORDER BY ?title"
}
