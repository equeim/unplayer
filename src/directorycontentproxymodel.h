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

#ifndef UNPLAYER_DIRECTORYCONTENTPROXYMODEL_H
#define UNPLAYER_DIRECTORYCONTENTPROXYMODEL_H

#include "filterproxymodel.h"

namespace unplayer
{
    class DirectoryContentProxyModel : public FilterProxyModel
    {
        Q_OBJECT
        Q_PROPERTY(int isDirectoryRole READ isDirectoryRole WRITE setIsDirectoryRole)
    public:
        int isDirectoryRole() const;
        void setIsDirectoryRole(int isDirectoryRole);

    protected:
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

    private:
        int mIsDirectoryRole = 0;
    };
}

#endif // UNPLAYER_DIRECTORYCONTENTPROXYMODEL_H
