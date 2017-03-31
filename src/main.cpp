/*
 * Unplayer
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

#include <memory>

#include <QGuiApplication>
#include <QItemSelectionModel>
#include <QQuickView>
#include <QQmlEngine>
#include <qqml.h>

#include <sailfishapp.h>

#include "albumsmodel.h"
#include "artistsmodel.h"
#include "directorycontentmodel.h"
#include "directorycontentproxymodel.h"
#include "directorytracksmodel.h"
#include "filterproxymodel.h"
#include "genresmodel.h"
#include "librarydirectoriesmodel.h"
#include "libraryutils.h"
#include "player.h"
#include "playlistmodel.h"
#include "playlistsmodel.h"
#include "playlistutils.h"
#include "queue.h"
#include "queuemodel.h"
#include "settings.h"
#include "trackinfo.h"
#include "tracksmodel.h"
#include "utils.h"

int main(int argc, char* argv[])
{
    using namespace unplayer;

    std::unique_ptr<QGuiApplication> app(SailfishApp::application(argc, argv));

    std::unique_ptr<QQuickView> view(SailfishApp::createView());

    Settings::instance();
    LibraryUtils::instance();

    const char* url = "harbour.unplayer";
    const int major = 0;
    const int minor = 1;

    qmlRegisterSingletonType<Settings>(url, major, minor, "Settings", [](QQmlEngine*, QJSEngine*) -> QObject* { return Settings::instance(); });
    qmlRegisterSingletonType<LibraryUtils>(url, major, minor, "LibraryUtils", [](QQmlEngine*, QJSEngine*) -> QObject* { return LibraryUtils::instance(); });

    qmlRegisterType<Player>(url, major, minor, "Player");

    qmlRegisterUncreatableType<Queue>(url, major, minor, "Queue", QString());
    qmlRegisterType<QueueModel>(url, major, minor, "QueueModel");

    qmlRegisterType<ArtistsModel>(url, major, minor, "ArtistsModel");
    qmlRegisterType<AlbumsModel>(url, major, minor, "AlbumsModel");
    qmlRegisterType<TracksModel>(url, major, minor, "TracksModel");
    qmlRegisterType<GenresModel>(url, major, minor, "GenresModel");

    qmlRegisterSingletonType<PlaylistUtils>(url, major, minor, "PlaylistUtils", [](QQmlEngine*, QJSEngine*) -> QObject* { return PlaylistUtils::instance(); });
    qmlRegisterType<PlaylistsModel>(url, major, minor, "PlaylistsModel");
    qmlRegisterType<PlaylistModel>(url, major, minor, "PlaylistModel");

    qmlRegisterType<DirectoryTracksModel>(url, major, minor, "DirectoryTracksModel");
    qmlRegisterType<DirectoryTracksProxyModel>(url, major, minor, "DirectoryTracksProxyModel");

    qmlRegisterType<DirectoryContentModel>(url, major, minor, "DirectoryContentModel");
    qmlRegisterType<DirectoryContentProxyModel>(url, major, minor, "DirectoryContentProxyModel");

    qmlRegisterType<FilterProxyModel>(url, major, minor, "FilterProxyModel");
    qmlRegisterType<QItemSelectionModel>();
    qmlRegisterType<QAbstractItemModel>();

    qmlRegisterSingletonType<Utils>(url, major, minor, "Utils", [](QQmlEngine*, QJSEngine*) -> QObject* { return new Utils(); });
    qmlRegisterType<TrackInfo>(url, major, minor, "TrackInfo");
    qmlRegisterType<LibraryDirectoriesModel>(url, major, minor, "LibraryDirectoriesModel");

    view->setSource(SailfishApp::pathTo(QLatin1String("qml/main.qml")));
    view->show();

    return app->exec();
}
