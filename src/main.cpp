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
#include <qqml.h>

#include <sailfishapp.h>

#include "directorytracksmodel.h"
#include "filepickermodel.h"
#include "filterproxymodel.h"
#include "player.h"
#include "playlistmodel.h"
#include "playlistutils.h"
#include "queue.h"
#include "queuemodel.h"
#include "utils.h"

static QObject *utils_singletontype_provider(QQmlEngine*, QJSEngine*)
{
    return new unplayer::Utils;
}

static QObject *playlistutils_singletontype_provider(QQmlEngine*, QJSEngine*)
{
    return new unplayer::PlaylistUtils;
}

int main(int argc, char *argv[])
{
    std::unique_ptr<QGuiApplication> app(SailfishApp::application(argc, argv));

    std::unique_ptr<QQuickView> view(SailfishApp::createView());

    const char* url = "harbour.unplayer";
    const int major = 0;
    const int minor = 1;

    qmlRegisterType<unplayer::Player>(url, major, minor, "Player");

    qmlRegisterType<unplayer::Queue>();
    qmlRegisterType<unplayer::QueueModel>(url, major, minor, "QueueModel");

    qmlRegisterType<unplayer::PlaylistModel>(url, major, minor, "PlaylistModel");
    qmlRegisterType<unplayer::DirectoryTracksModel>(url, major, minor, "DirectoryTracksModel");
    qmlRegisterType<unplayer::DirectoryTracksProxyModel>(url, major, minor, "DirectoryTracksProxyModel");
    qmlRegisterType<unplayer::FilePickerModel>(url, major, minor, "FilePickerModel");

    qmlRegisterType<unplayer::FilterProxyModel>(url, major, minor, "FilterProxyModel");

    qmlRegisterType<QItemSelectionModel>();
    qmlRegisterType<QAbstractItemModel>();

    qmlRegisterSingletonType<unplayer::Utils>(url, major, minor, "Utils", utils_singletontype_provider);
    qmlRegisterSingletonType<unplayer::PlaylistUtils>(url, major, minor, "PlaylistUtils", playlistutils_singletontype_provider);

    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->show();

    return app->exec();
}
