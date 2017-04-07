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

#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <qqml.h>

#include <sailfishapp.h>

#include "libraryutils.h"
#include "player.h"
#include "queue.h"
#include "settings.h"
#include "utils.h"

using namespace unplayer;

int main(int argc, char* argv[])
{
    const std::unique_ptr<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setApplicationVersion(QLatin1String(UNPLAYER_VERSION));

    QCommandLineParser parser;
    parser.addPositionalArgument(QLatin1String("files"), QLatin1String("Music files"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app->arguments());

    const QDBusConnection connection(QDBusConnection::sessionBus());
    if (connection.interface()->isServiceRegistered(QLatin1String("org.equeim.unplayer"))) {
        QDBusMessage message(QDBusMessage::createMethodCall(QLatin1String("org.equeim.unplayer"),
                                                            QLatin1String("/org/equeim/unplayer"),
                                                            QLatin1String("org.equeim.unplayer"),
                                                            QLatin1String("addTracksToQueue")));
        message.setArguments(Utils::parseArguments(parser.positionalArguments()));
        connection.call(message);
        return 0;
    }

    const std::unique_ptr<QQuickView> view(SailfishApp::createView());

    view->rootContext()->setContextProperty(QLatin1String("commandLineArguments"), Utils::parseArguments(parser.positionalArguments()));

    Settings::instance();
    LibraryUtils::instance();
    Utils::registerTypes();

    view->engine()->addImageProvider(QueueImageProvider::providerId, new QueueImageProvider(Player::instance()->queue()));

    view->setSource(SailfishApp::pathTo(QLatin1String("qml/main.qml")));
    view->show();

    return app->exec();
}
