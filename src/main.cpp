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

#include <memory>

#include <QGuiApplication>
#include <QItemSelectionModel>
#include <QQuickView>
#include <qqml.h>

#include <sailfishapp.h>

#include <gst/gst.h>

#include "directorytracksmodel.h"
#include "filterproxymodel.h"
#include "player.h"
#include "playlistmodel.h"
#include "playlistutils.h"
#include "queue.h"
#include "queuemodel.h"
#include "utils.h"
#include "folderlistmodel/qquickfolderlistmodel.h"

static QObject *utils_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    return new Unplayer::Utils;
}

static QObject *playlistutils_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    return new Unplayer::PlaylistUtils;
}

int main(int argc, char *argv[])
{
    std::unique_ptr<QGuiApplication> app(SailfishApp::application(argc, argv));
    gst_init(&argc, &argv);

    std::unique_ptr<QQuickView> view(SailfishApp::createView());

    qmlRegisterType<Unplayer::Player>("harbour.unplayer", 0, 1, "Player");

    qmlRegisterType<Unplayer::Queue>();
    qRegisterMetaType<Unplayer::Queue*>("Queue*");

    qmlRegisterType<Unplayer::QueueModel>("harbour.unplayer", 0, 1, "QueueModel");
    qmlRegisterType<Unplayer::PlaylistModel>("harbour.unplayer", 0, 1, "PlaylistModel");
    qmlRegisterType<Unplayer::DirectoryTracksModel>("harbour.unplayer", 0, 1, "DirectoryTracksModel");
    qmlRegisterType<Unplayer::DirectoryTracksProxyModel>("harbour.unplayer", 0, 1, "DirectoryTracksProxyModel");

    qmlRegisterType<Unplayer::FilterProxyModel>("harbour.unplayer", 0, 1, "FilterProxyModel");

    qmlRegisterType<QQuickFolderListModel, 1>("harbour.unplayer", 0, 1, "FolderListModel");
    qmlRegisterType<QItemSelectionModel>();
    qmlRegisterType<QAbstractItemModel>();

    qmlRegisterSingletonType<Unplayer::Utils>("harbour.unplayer", 0, 1, "Utils", utils_singletontype_provider);
    qmlRegisterSingletonType<Unplayer::PlaylistUtils>("harbour.unplayer", 0, 1, "PlaylistUtils", playlistutils_singletontype_provider);

    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->show();

    return app->exec();
}
