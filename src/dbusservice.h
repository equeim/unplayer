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

#ifndef UNPLAYER_DBUSSERVICE_H
#define UNPLAYER_DBUSSERVICE_H

#include <QObject>

class OrgFreedesktopApplicationAdaptor;
class OrgEqueimUnplayerAdaptor;

namespace unplayer
{
    class DBusService : public QObject
    {
        Q_OBJECT
    public:
        static const QLatin1String serviceName;
        static const QLatin1String objectPath;

        DBusService();

    private:
        friend OrgFreedesktopApplicationAdaptor;
        void Activate(const QVariantMap& platform_data);
        void Open(const QStringList& uris, const QVariantMap& platform_data);
        void ActivateAction(const QString& action_name, const QVariantList& parameter, const QVariantMap& platform_data);

        friend OrgEqueimUnplayerAdaptor;
        void addTracksToQueue(const QStringList &tracks);

    signals:
        void windowActivationRequested();
        void filesOpeningRequested(const QStringList& uris);
    };
}

#endif // UNPLAYER_DBUSSERVICE_H
