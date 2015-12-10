/*
 * Unplayer
 * Copyright (C) 2015 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtSparql 1.0

import harbour.unplayer 0.1 as Unplayer

SparqlListModel {
    property bool allArtists
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

        if (artist.length === 0)
            queryString += "    FILTER(!bound(?rawArtist)).\n"
        else
            queryString += "    FILTER(?rawArtist = \"" + Unplayer.Utils.escapeSparql(artist) + "\").\n"

        queryString += "}"

        return queryString
    }
}
