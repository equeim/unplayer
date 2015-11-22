#include <memory>

#include <QGuiApplication>
#include <QQuickView>
#include <qqml.h>

#include <sailfishapp.h>

#include <gst/gst.h>

#include "filterproxymodel.h"
#include "player.h"
#include "playlistmodel.h"
#include "playlistutils.h"
#include "queue.h"
#include "queuemodel.h"
#include "utils.h"

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
    qmlRegisterType<Unplayer::FilterProxyModel>("harbour.unplayer", 0, 1, "FilterProxyModel");
    qmlRegisterType<QAbstractItemModel>();

    qmlRegisterSingletonType<Unplayer::Utils>("harbour.unplayer", 0, 1, "Utils", utils_singletontype_provider);
    qmlRegisterSingletonType<Unplayer::PlaylistUtils>("harbour.unplayer", 0, 1, "PlaylistUtils", playlistutils_singletontype_provider);

    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->show();

    return app->exec();
}
