/*
 * Unplayer
 * Copyright (C) 2015-2019 Alexey Rochev <equeim@gmail.com>
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
#include <QDBusInterface>
#include <QDBusMessage>
#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <qqml.h>

#include <sailfishapp.h>

#include "commandlineparser.h"
#include "libraryutils.h"
#include "player.h"
#include "queue.h"
#include "settings.h"
#include "signalhandler.h"
#include "utils.h"

using namespace unplayer;

int main(int argc, char* argv[])
{
    SignalHandler::setupHandlers();

    const auto args(CommandLineArgs::parse(argc, argv));
    if (args.exit) {
        return args.returnCode;
    }

    if (SignalHandler::exitRequested) {
        return 0;
    }

    if (args.resetLibrary || args.updateLibrary) {
        QCoreApplication app(argc, argv);
        QCoreApplication::setOrganizationName(QCoreApplication::applicationName());
        QCoreApplication::setOrganizationDomain(QCoreApplication::applicationName());

        if (args.resetLibrary) {
            LibraryUtils::instance()->resetDatabase();
            if (!args.updateLibrary) {
                return 0;
            }
        }

        Settings::instance();

        if (SignalHandler::exitRequested) {
            return 0;
        }

        QObject::connect(LibraryUtils::instance(), &LibraryUtils::updatingChanged, &app, []() {
            if (!LibraryUtils::instance()->isUpdating()) {
                QCoreApplication::quit();
            }
        });

        LibraryUtils::instance()->updateDatabase();

        if (SignalHandler::exitRequested) {
            return 0;
        }

        return QCoreApplication::exec();
    }

    {
        QDBusInterface interface(QLatin1String("org.equeim.unplayer"),
                                 QLatin1String("/org/equeim/unplayer"),
                                 QLatin1String("org.equeim.unplayer"));
        if (interface.isValid()) {
            const QDBusMessage reply(interface.call(QLatin1String("addTracksToQueue"), args.files));
            if (reply.type() != QDBusMessage::ReplyMessage) {
                qWarning() << "D-Bus method call failed, error string:" << reply.errorMessage();
            }
            return 0;
        }
    }

    const std::unique_ptr<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setApplicationVersion(QLatin1String(UNPLAYER_VERSION));

    if (SignalHandler::exitRequested) {
        return 0;
    }

    const std::unique_ptr<QQuickView> view(SailfishApp::createView());

    view->rootContext()->setContextProperty(QLatin1String("commandLineArguments"), args.files);

    Settings::instance();
    LibraryUtils::instance();
    Utils::registerTypes();

    view->engine()->addImageProvider(QueueImageProvider::providerId, new QueueImageProvider(Player::instance()->queue()));

    if (SignalHandler::exitRequested) {
        return 0;
    }

    view->setSource(SailfishApp::pathTo(QLatin1String("qml/main.qml")));
    view->show();

    if (SignalHandler::exitRequested) {
        return 0;
    }
    SignalHandler::setupNotifier();

    return QCoreApplication::exec();
}
