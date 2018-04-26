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

#include "filterproxymodel.h"

#include <algorithm>
#include <QItemSelectionModel>

namespace unplayer
{
    FilterProxyModel::FilterProxyModel()
        : mSortEnabled(false),
          mSelectionModel(new QItemSelectionModel(this))
    {
        mCollator.setNumericMode(true);
        QObject::connect(mSelectionModel, &QItemSelectionModel::selectionChanged, this, &FilterProxyModel::selectionChanged);
    }

    void FilterProxyModel::classBegin()
    {

    }

    void FilterProxyModel::componentComplete()
    {
        if (mSortEnabled) {
            sort(0);
        }
    }

    QVector<int> FilterProxyModel::sourceIndexes() const
    {
        QVector<int> indexes;
        indexes.reserve(rowCount());
        for (int i = 0, max = rowCount(); i < max; ++i) {
            indexes.append(sourceIndex(i));
        }
        return indexes;
    }

    bool FilterProxyModel::isSortEnabled() const
    {
        return mSortEnabled;
    }

    void FilterProxyModel::setSortEnabled(bool sortEnabled)
    {
        mSortEnabled = sortEnabled;
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

    bool FilterProxyModel::hasSelection() const
    {
        return mSelectionModel->hasSelection();
    }

    int FilterProxyModel::selectedIndexesCount() const
    {
        return mSelectionModel->selectedIndexes().size();
    }

    QVector<int> FilterProxyModel::selectedSourceIndexes() const
    {
        QModelIndexList modelIndexes(mSelectionModel->selectedIndexes());
        std::sort(modelIndexes.begin(), modelIndexes.end());

        QVector<int> indexes;
        indexes.reserve(modelIndexes.size());
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

    bool FilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        const QVariant leftVariant(left.data(sortRole()));
        if (leftVariant.type() == QVariant::String) {
            return (mCollator.compare(leftVariant.toString(), right.data(sortRole()).toString()) < 0);
        }
        return QSortFilterProxyModel::lessThan(left, right);
    }
}
