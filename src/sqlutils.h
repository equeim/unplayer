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

#ifndef UNPLAYER_SQLUTILS_H
#define UNPLAYER_SQLUTILS_H

#include <QSqlDatabase>
#include <QSqlQuery>

namespace unplayer
{
    struct DatabaseConnectionGuard
    {
        inline ~DatabaseConnectionGuard()
        {
            QSqlDatabase::removeDatabase(connectionName);
        }
        const QString connectionName;
    };

    struct TransactionGuard
    {
        inline TransactionGuard(QSqlDatabase& db) : db(db)
        {
            db.transaction();
        }

        inline ~TransactionGuard()
        {
            db.commit();
        }
        QSqlDatabase& db;
    };

    template<typename C>
    inline typename C::size_type reserveFromQuery(C& container, QSqlQuery& query)
    {
        if (query.last()) {
            const auto size = static_cast<typename C::size_type>(query.at() + 1);
            container.reserve(size);
            query.seek(QSql::BeforeFirstRow);
            return size;
        }
        return 0;
    }
}

#endif // UNPLAYER_SQLUTILS_H
