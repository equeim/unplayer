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

#include <vector>

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

    template<typename C>
    inline typename C::size_type reserveFromQueryAppend(C& container, QSqlQuery& query)
    {
        if (query.last()) {
            const auto size = static_cast<typename C::size_type>(query.at() + 1);
            container.reserve(container.size() + size);
            query.seek(QSql::BeforeFirstRow);
            return size;
        }
        return 0;
    }

    inline QString makeInStringFromIds(const std::vector<int>& ids, size_t first, size_t count, bool& addNull)
    {
        QString str(QString::number(ids[first]));
        for (size_t i = first + 1, max = first + count; i < max; ++i) {
            const int id = ids[i];
            if (id > 0) {
                str += QLatin1Char(',');
                str += QString::number(id);
            } else {
                addNull = true;
            }
        }
        str += QLatin1Char(')');
        return str;
    }

    inline QString makeInStringFromIds(const std::vector<int>& ids, size_t first, size_t count)
    {
        bool addNull;
        return makeInStringFromIds(ids, first, count, addNull);
    }

    inline QString makeInStringForParameters(size_t count)
    {
        QString str(QLatin1Char('?'));
        const auto add(QStringLiteral(",?"));
        const int addSize = add.size();
        str.reserve(str.size() + static_cast<int>(count - 1) * addSize + 1);
        for (size_t i = 1; i < count; ++i) {
            str += add;
        }
        str += QLatin1Char(')');
        return str;
    }
}

#endif // UNPLAYER_SQLUTILS_H
