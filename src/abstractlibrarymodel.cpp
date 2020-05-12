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

#include <memory>

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QtConcurrentRun>

#include "libraryutils.h"
#include "sqlutils.h"
#include "utilsfunctions.h"

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

        setLoading(true);

        auto queryString(makeQueryString());
        auto itemFactory = createItemFactory();
        auto future(QtConcurrent::run([itemFactory, queryString = std::move(queryString)] {
            auto items(std::make_shared<std::vector<Item>>());

            std::unique_ptr<AbstractItemFactory> itemFactoryUnique(itemFactory);

            DatabaseConnectionGuard databaseGuard{QUuid::createUuid().toString()};
            auto db(LibraryUtils::openDatabase(databaseGuard.connectionName));

            QSqlQuery query(db);
            if (!query.prepare(queryString)) {
                qWarning() << "Prepare failed:" << query.lastError();
                return items;
            }

            if (query.exec()) {
                if (reserveFromQuery(*items, query) > 0) {
                    while (query.next()) {
                        items->push_back(itemFactory->itemFromQuery(query));
                    }
                }
            } else {
                qWarning() << "Exec failed: " << query.lastError();
            }

            return items;
        }));

        onFutureFinished(future, this, [this](std::shared_ptr<std::vector<Item>>&& items) {
            removeRows(0, rowCount());
            if (!items->empty()) {
                beginInsertRows(QModelIndex(), 0, static_cast<int>(items->size()) - 1);
                mItems = std::move(*items);
                endInsertRows();
            }
            setLoading(false);
        });
    }
}
