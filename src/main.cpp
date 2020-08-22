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

#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>

#include <sailfishapp.h>

#include "commandlineparser.h"
#include "dbusservice.h"
#include "libraryutils.h"
#include "mediaartutils.h"
#include "player.h"
#include "queue.h"
#include "settings.h"
#include "signalhandler.h"
#include "utils.h"

#include "org_freedesktop_application_interface.h"
#include "org_equeim_unplayer_interface.h"

using namespace unplayer;

namespace
{
    int handleNoGuiCommandLineArguments(int& argc, char** argv, const CommandLineArgs& args)
    {
        QCoreApplication app(argc, argv);
        QCoreApplication::setOrganizationName(QCoreApplication::applicationName());
        QCoreApplication::setOrganizationDomain(QCoreApplication::applicationName());

        if (args.resetLibrary) {
            LibraryUtils::instance()->resetDatabase();
            if (!args.updateLibrary) {
                return EXIT_SUCCESS;
            }
        }

        Settings::instance();
        MediaArtUtils::instance();

        if (SignalHandler::exitRequested) {
            return EXIT_SUCCESS;
        }

        QObject::connect(LibraryUtils::instance(), &LibraryUtils::updatingChanged, &app, []() {
            if (!LibraryUtils::instance()->isUpdating()) {
                QCoreApplication::quit();
            }
        });

        if (!LibraryUtils::instance()->updateDatabase()) {
            qWarning("Failed to start library update");
            return EXIT_FAILURE;
        }

        if (SignalHandler::exitRequested) {
            return EXIT_SUCCESS;
        }
        SignalHandler::setupNotifier();
        return QCoreApplication::exec();
    }

    // Returns true if another instance exists
    bool tryToInvokeExistingInstance(const CommandLineArgs& args)
    {
        const auto waitForReply = [](QDBusPendingReply<>&& pending) {
            pending.waitForFinished();
            const auto reply(pending.reply());
            if (reply.type() != QDBusMessage::ReplyMessage) {
                qWarning() << "D-Bus method call failed, error string:" << reply.errorMessage();
                return false;
            }
            return true;
        };

        OrgFreedesktopApplicationInterface interface(DBusService::serviceName, DBusService::objectPath, QDBusConnection::sessionBus());
        if (interface.isValid()) {
            qInfo("Only one instance of Unplayer can be run at the same time");
            if (args.files.isEmpty()) {
                qInfo("Requesting window activation");
                if (waitForReply(interface.Activate({}))) {
                    return true;
                }
            } else {
                qInfo("Requesting files opening");
                if (waitForReply(interface.Open(args.files, {}))) {
                    return true;
                }
            }

            if (!args.files.isEmpty()) {
                qWarning("Trying deprecated interface");
                OrgEqueimUnplayerInterface deprecatedInterface(DBusService::serviceName, DBusService::objectPath, QDBusConnection::sessionBus());
                if (deprecatedInterface.isValid()) {
                    qInfo("Requesting files opening");
                    waitForReply(deprecatedInterface.addTracksToQueue(args.files));
                }
            }

            return true;
        }
        return false;
    }
}

int main(int argc, char** argv)
{
    SignalHandler::setupHandlers();

    const auto args(CommandLineArgs::parse(argc, argv));
    if (args.exit) {
        return args.returnCode;
    }

    if (SignalHandler::exitRequested) {
        return EXIT_SUCCESS;
    }

    if (args.resetLibrary || args.updateLibrary) {
        return handleNoGuiCommandLineArguments(argc, argv, args);
    }

    if (tryToInvokeExistingInstance(args)) {
        return EXIT_SUCCESS;
    }

    const std::unique_ptr<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setApplicationVersion(QLatin1String(UNPLAYER_VERSION));

    if (SignalHandler::exitRequested) {
        return EXIT_SUCCESS;
    }

    const std::unique_ptr<QQuickView> view(SailfishApp::createView());

    view->rootContext()->setContextProperty(QLatin1String("commandLineArguments"), args.files);

    Settings::instance();
    MediaArtUtils::instance();
    LibraryUtils::instance();
    Utils::registerTypes();

    view->engine()->addImageProvider(QueueImageProvider::providerId, new QueueImageProvider(Player::instance()->queue()));

    if (SignalHandler::exitRequested) {
        return EXIT_SUCCESS;
    }

    view->setSource(SailfishApp::pathTo(QLatin1String("qml/main.qml")));
    view->show();

    if (SignalHandler::exitRequested) {
        return EXIT_SUCCESS;
    }
    SignalHandler::setupNotifier();

    return QCoreApplication::exec();
}
