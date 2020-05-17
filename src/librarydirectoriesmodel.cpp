/*
 * Unplayer
 * Copyright (C) 2015-2019 Alexey Rochev <equeim@gmail.com>
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

#include "modelutils.h"
#include "settings.h"

namespace unplayer
{
    void LibraryDirectoriesModel::classBegin()
    {

    }

    void LibraryDirectoriesModel::componentComplete()
    {
        switch (mType) {
        case Library:
            mDirectories = Settings::instance()->libraryDirectories();
            break;
        case Blacklisted:
            mDirectories = Settings::instance()->blacklistedDirectories();
            break;
        default:
            break;
        }
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

    bool LibraryDirectoriesModel::removeRows(int row, int count, const QModelIndex& parent)
    {
        if (count > 0 && (row + count) <= static_cast<int>(mDirectories.size())) {
            beginRemoveRows(parent, row, row + count - 1);
            const auto first(mDirectories.begin() + row);
            mDirectories.erase(first, first + count);
            endRemoveRows();
            return true;
        }
        return false;
    }

    LibraryDirectoriesModel::Type LibraryDirectoriesModel::type() const
    {
        return mType;
    }

    void LibraryDirectoriesModel::setType(Type type)
    {
        mType = type;
    }

    void LibraryDirectoriesModel::addDirectory(const QString& directory)
    {
        beginInsertRows(QModelIndex(), mDirectories.size(), mDirectories.size());
        mDirectories.push_back(directory);
        endInsertRows();

        switch (mType) {
        case Library:
            Settings::instance()->setLibraryDirectories(mDirectories);
            break;
        case Blacklisted:
            Settings::instance()->setBlacklistedDirectories(mDirectories);
            break;
        default:
            break;
        }
    }

    void LibraryDirectoriesModel::removeDirectory(int index)
    {
        beginRemoveRows(QModelIndex(), index, index);
        mDirectories.removeAt(index);
        endRemoveRows();

        switch (mType) {
        case Library:
            Settings::instance()->setLibraryDirectories(mDirectories);
            break;
        case Blacklisted:
            Settings::instance()->setBlacklistedDirectories(mDirectories);
            break;
        default:
            break;
        }
    }

    void LibraryDirectoriesModel::removeDirectories(const std::vector<int>& indexes)
    {
        ModelBatchRemover::removeIndexes(this, indexes);

        switch (mType) {
        case Library:
            Settings::instance()->setLibraryDirectories(mDirectories);
            break;
        case Blacklisted:
            Settings::instance()->setBlacklistedDirectories(mDirectories);
            break;
        default:
            break;
        }
    }

    QHash<int, QByteArray> LibraryDirectoriesModel::roleNames() const
    {
        return {{Qt::UserRole, "directory"}};
    }
}
