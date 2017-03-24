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

#include "filterproxymodel.h"

#include <algorithm>

#include <QItemSelectionModel>

namespace unplayer
{
    FilterProxyModel::FilterProxyModel()
        : mSelectionModel(new QItemSelectionModel(this))
    {
        QObject::connect(mSelectionModel, &QItemSelectionModel::selectionChanged, this, &FilterProxyModel::selectionChanged);
    }

    void FilterProxyModel::classBegin()
    {
    }

    void FilterProxyModel::componentComplete()
    {
        setFilterRole(sourceModel()->roleNames().key(mFilterRoleName));
    }

    QByteArray FilterProxyModel::filterRoleName() const
    {
        return mFilterRoleName;
    }

    void FilterProxyModel::setFilterRoleName(const QByteArray& filterRoleName)
    {
        mFilterRoleName = filterRoleName;
    }

    int FilterProxyModel::count() const
    {
        return rowCount();
    }

    int FilterProxyModel::proxyIndex(int sourceIndex) const
    {
        return mapFromSource(sourceModel()->index(sourceIndex, 0)).row();
    }

    int FilterProxyModel::sourceIndex(int proxyIndex) const
    {
        return mapToSource(index(proxyIndex, 0)).row();
    }

    QItemSelectionModel* FilterProxyModel::selectionModel() const
    {
        return mSelectionModel;
    }

    int FilterProxyModel::selectedIndexesCount() const
    {
        return mSelectionModel->selectedIndexes().size();
    }

    QList<int> FilterProxyModel::selectedSourceIndexes() const
    {
        QList<int> indexes;

        QModelIndexList modelIndexes(mSelectionModel->selectedIndexes());
        std::sort(modelIndexes.begin(), modelIndexes.end());

        for (const QModelIndex& index : modelIndexes) {
            indexes.append(index.row());
        }

        return indexes;
    }

    bool FilterProxyModel::isSelected(int row) const
    {
        return mSelectionModel->isSelected(index(row, 0));
    }

    void FilterProxyModel::select(int row)
    {
        mSelectionModel->select(index(row, 0), QItemSelectionModel::Toggle);
    }

    void FilterProxyModel::selectAll()
    {
        mSelectionModel->select(QItemSelection(index(0, 0), index(rowCount() - 1, 0)), QItemSelectionModel::Select);
    }
}
