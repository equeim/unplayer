import Sailfish.Silica 1.0
import QtSparql 1.0

import harbour.unplayer 0.1 as Unplayer

import "components"

ApplicationWindow
{
    property bool largeScreen: Screen.sizeCategory === Screen.Large ||
                               Screen.sizeCategory === Screen.ExtraLarge

    _defaultPageOrientations: Orientation.All
    allowedOrientations: defaultAllowedOrientations
    bottomMargin: nowPlayingPanel.visibleSize
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    initialPage: Qt.resolvedUrl("pages/MainPage.qml")

    Unplayer.Player {
        id: player
    }

    SparqlConnection {
        id: sparqlConnection
        property bool ready: status === SparqlConnection.Ready
        driver: "QTRACKER_DIRECT"
    }

    NowPlayingPanel {
        id: nowPlayingPanel
    }
}
