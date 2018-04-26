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

#include "databasemodel.h"

#include <QDebug>
#include <QSqlError>

namespace unplayer
{
    DatabaseModel::DatabaseModel()
        : mQuery(new QSqlQuery()),
          mRowCount(0)
    {

    }

    void DatabaseModel::classBegin()
    {

    }

    void DatabaseModel::componentComplete()
    {

    }

    int DatabaseModel::rowCount(const QModelIndex&) const
    {
        return mRowCount;
    }

    void DatabaseModel::execQuery()
    {
        if (mQuery->exec()) {
            mQuery->last();
            mRowCount = mQuery->at() + 1;
        } else {
            qWarning() << mQuery->lastError();
        }
    }
}
