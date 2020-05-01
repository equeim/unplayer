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

#ifndef UNPLAYER_ABSTRACTLIBRARYMODEL_H
#define UNPLAYER_ABSTRACTLIBRARYMODEL_H

#include <vector>

#include "asyncloadingmodel.h"

class QSqlQuery;

namespace unplayer
{
    template<typename Item>
    class AbstractLibraryModel : public AsyncLoadingModel
    {
    public:
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

    protected:
        class AbstractItemFactory
        {
        public:
            virtual ~AbstractItemFactory() = default;
            virtual Item itemFromQuery(const QSqlQuery& query) = 0;
        };

        void execQuery();
        virtual QString makeQueryString() = 0;
        virtual AbstractItemFactory* createItemFactory() = 0;

        std::vector<Item> mItems;
    };
}

#endif // UNPLAYER_ABSTRACTLIBRARYMODEL_H
