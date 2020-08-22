/*
 * Unplayer
 * Copyright (C) 2015-2020 Alexey Rochev <equeim@gmail.com>
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

#include "dbusservice.h"

#include "org_freedesktop_application_adaptor.h"
#include "org_equeim_unplayer_adaptor.h"

namespace unplayer
{
    const QLatin1String DBusService::serviceName("org.equeim.unplayer");
    const QLatin1String DBusService::objectPath("/org/equeim/unplayer");

    DBusService::DBusService()
    {
        new OrgFreedesktopApplicationAdaptor(this);
        new OrgEqueimUnplayerAdaptor(this);

        auto connection(QDBusConnection::sessionBus());
        if (connection.registerService(serviceName)) {
            qInfo("Registered D-Bus service");
            if (connection.registerObject(objectPath, this)) {
                qInfo("Registered D-Bus object");
            } else {
                qWarning() << "Failed to register D-Bus object" << connection.lastError();
            }
        } else {
            qWarning() << "Failed toregister D-Bus service" << connection.lastError();
        }
    }

    void DBusService::Activate(const QVariantMap& platform_data)
    {
        qInfo().nospace() << "Window activation requested, platform_data=" << platform_data;
        emit windowActivationRequested();
    }

    void DBusService::Open(const QStringList& uris, const QVariantMap& platform_data)
    {
        qInfo().nospace() << "Files opening requested, uris=" << uris << ", platform_data=" << platform_data;
        emit filesOpeningRequested(uris);
    }

    void DBusService::ActivateAction(const QString& action_name, const QVariantList& parameter, const QVariantMap& platform_data)
    {
        qInfo().nospace() << "Action activated, action_name=" << action_name << ", parameter=" << parameter << ", platform_data=" << platform_data;
    }

    void DBusService::addTracksToQueue(const QStringList& tracks)
    {
        qInfo().nospace() << "Files opening requested, tracks=" << tracks;
        emit filesOpeningRequested(tracks);
    }
}
