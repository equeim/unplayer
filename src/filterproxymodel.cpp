/*
 * Unplayer
 * Copyright (C) 2015 Alexey Rochev <equeim@gmail.com>
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

namespace Unplayer
{

FilterProxyModel::FilterProxyModel()
    : m_selectionModel(new QItemSelectionModel(this))
{
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged, this, &FilterProxyModel::selectionChanged);
}

void FilterProxyModel::classBegin()
{

}

void FilterProxyModel::componentComplete()
{
    setFilterRole(sourceModel()->roleNames().key(m_filterRoleName));
}


QByteArray FilterProxyModel::filterRoleName() const
{
    return m_filterRoleName;
}

void FilterProxyModel::setFilterRoleName(const QByteArray &filterRoleName)
{
    m_filterRoleName = filterRoleName;
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
    return m_selectionModel;
}

int FilterProxyModel::selectedIndexesCount() const
{
    return m_selectionModel->selectedIndexes().size();
}

QList<int> FilterProxyModel::selectedSourceIndexes() const
{
    QList<int> indexes;

    QModelIndexList modelIndexes(m_selectionModel->selectedIndexes());
    std::sort(modelIndexes.begin(), modelIndexes.end());

    for (const QModelIndex &index : modelIndexes)
        indexes.append(index.row());

    return indexes;
}

bool FilterProxyModel::isSelected(int row) const
{
    return m_selectionModel->isSelected(index(row, 0));
}

void FilterProxyModel::select(int row)
{
    m_selectionModel->select(index(row, 0), QItemSelectionModel::Toggle);
}

void FilterProxyModel::selectAll()
{
    m_selectionModel->select(QItemSelection(index(0, 0), index(rowCount() - 1, 0)), QItemSelectionModel::Select);
}

}
