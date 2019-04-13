/*
 * Unplayer
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
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

#include <atomic>
#include <csignal>
#include <iostream>
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

#include "clara.hpp"

#include "libraryutils.h"
#include "player.h"
#include "queue.h"
#include "settings.h"
#include "utils.h"

static std::atomic_bool exitRequested;
void signalHandler(int)
{
    exitRequested = true;
    QCoreApplication::quit();
}

using namespace unplayer;

int main(int argc, char* argv[])
{
    std::signal(SIGQUIT, signalHandler);
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGHUP, signalHandler);

    std::vector<std::string> files;
    {
        using namespace clara;

        bool version = false;
        bool help = false;
        auto cli = Opt(version)["-v"]["--version"]("display version information") | Help(help) | Arg(files, "files");
        auto result = cli.parse(Args(argc, argv));
        if (!result) {
            std::cerr << result.errorMessage() << std::endl;
            return 1;
        }
        if (version) {
            std::cout << cli.m_exeName.name() << ' ' << UNPLAYER_VERSION << std::endl;
            return 0;
        }
        if (help) {
            std::cout << cli << std::endl;
            return 0;
        }
    }

    if (exitRequested) {
        return 0;
    }

    {
        const QDBusConnection connection(QDBusConnection::sessionBus());
        if (connection.interface()->isServiceRegistered(QLatin1String("org.equeim.unplayer"))) {
            QDBusMessage message(QDBusMessage::createMethodCall(QLatin1String("org.equeim.unplayer"),
                                                                QLatin1String("/org/equeim/unplayer"),
                                                                QLatin1String("org.equeim.unplayer"),
                                                                QLatin1String("addTracksToQueue")));
            message.setArguments({Utils::processArguments(files)});
            connection.call(message);
            return 0;
        }
    }

    const std::unique_ptr<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setApplicationVersion(QLatin1String(UNPLAYER_VERSION));

    if (exitRequested) {
        return 0;
    }

    const std::unique_ptr<QQuickView> view(SailfishApp::createView());

    view->rootContext()->setContextProperty(QLatin1String("commandLineArguments"), Utils::processArguments(files));

    Settings::instance();
    LibraryUtils::instance();
    Utils::registerTypes();

    view->engine()->addImageProvider(QueueImageProvider::providerId, new QueueImageProvider(Player::instance()->queue()));

    if (exitRequested) {
        return 0;
    }

    view->setSource(SailfishApp::pathTo(QLatin1String("qml/main.qml")));
    view->show();

    if (exitRequested) {
        return 0;
    }

    return app->exec();
}
