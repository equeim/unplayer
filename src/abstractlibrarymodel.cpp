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

#include "abstractlibrarymodel.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

namespace unplayer
{
    template<typename Item>
    int AbstractLibraryModel<Item>::rowCount(const QModelIndex&) const
    {
        return static_cast<int>(mItems.size());
    }

    template<typename Item>
    bool AbstractLibraryModel<Item>::removeRows(int row, int count, const QModelIndex& parent)
    {
        if (count > 0 && (row + count) <= static_cast<int>(mItems.size())) {
            beginRemoveRows(parent, row, row + count - 1);
            const auto first(mItems.begin() + row);
            mItems.erase(first, first + count);
            endRemoveRows();
            return true;
        }
        return false;
    }

    template<typename Item>
    void AbstractLibraryModel<Item>::execQuery()
    {
        removeRows(0, rowCount());

        QSqlQuery query;
        if (!query.prepare(makeQueryString())) {
            qWarning() << __func__ << "prepare failed:" << query.lastError();
            return;
        }

        if (query.exec()) {
            if (query.last()) {
                const int lastIndex = query.at();
                query.seek(QSql::BeforeFirstRow);
                beginInsertRows(QModelIndex(), 0, lastIndex);
                mItems.reserve(static_cast<size_t>(lastIndex + 1));
                while (query.next()) {
                    mItems.emplace_back(itemFromQuery(query));
                }
                endInsertRows();
            }
        } else {
            qWarning() << query.lastError();
        }
    }
}
