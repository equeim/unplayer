/*
 * Unplayer
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

#ifndef UNPLAYER_DATABASEMODEL_H
#define UNPLAYER_DATABASEMODEL_H

#include <memory>

#include <QAbstractListModel>
#include <QQmlParserStatus>
#include <QSqlQuery>

namespace unplayer
{
    class DatabaseModel : public QAbstractListModel, public QQmlParserStatus
    {
        Q_OBJECT
        Q_INTERFACES(QQmlParserStatus)
    public:
        DatabaseModel();
        void classBegin() override;
        void componentComplete() override;
        int rowCount(const QModelIndex& parent) const override;
    protected:
        void execQuery();

        std::unique_ptr<QSqlQuery> mQuery;
        int mRowCount;
    };
}

#endif // UNPLAYER_DATABASEMODEL_H
