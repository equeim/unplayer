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
#include "filterproxymodel.moc"

namespace Unplayer
{

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

void FilterProxyModel::setFilterRoleName(const QByteArray &newFilterRoleName)
{
    m_filterRoleName = newFilterRoleName;
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

}
