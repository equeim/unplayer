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

#include "librarydirectoriesmodel.h"

#include <algorithm>

#include "settings.h"

namespace unplayer
{
    LibraryDirectoriesModel::LibraryDirectoriesModel()
        : mDirectories(Settings::instance()->libraryDirectories())
    {

    }

    QVariant LibraryDirectoriesModel::data(const QModelIndex& index, int role) const
    {
        if (role == Qt::UserRole) {
            return mDirectories.at(index.row());
        }
        return QVariant();
    }

    int LibraryDirectoriesModel::rowCount(const QModelIndex&) const
    {
        return mDirectories.size();
    }

    void LibraryDirectoriesModel::addDirectory(const QString& directory)
    {
        beginInsertRows(QModelIndex(), mDirectories.size(), mDirectories.size());
        mDirectories.append(directory);
        endInsertRows();
        Settings::instance()->setLibraryDirectories(mDirectories);
    }

    void LibraryDirectoriesModel::removeDirectory(int index)
    {
        beginRemoveRows(QModelIndex(), index, index);
        mDirectories.removeAt(index);
        endRemoveRows();
        Settings::instance()->setLibraryDirectories(mDirectories);
    }

    void LibraryDirectoriesModel::removeDirectories(QVector<int> indexes)
    {
        std::reverse(indexes.begin(), indexes.end());
        for (int index : indexes) {
            beginRemoveRows(QModelIndex(), index, index);
            mDirectories.removeAt(index);
            endRemoveRows();
        }
        Settings::instance()->setLibraryDirectories(mDirectories);
    }

    QHash<int, QByteArray> LibraryDirectoriesModel::roleNames() const
    {
        return {{Qt::UserRole, "directory"}};
    }
}
