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

#ifndef UNPLAYER_FILTERPROXYMODEL_H
#define UNPLAYER_FILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QQmlParserStatus>

class QItemSelectionModel;

namespace unplayer
{
    class FilterProxyModel : public QSortFilterProxyModel, public QQmlParserStatus
    {
        Q_OBJECT
        Q_INTERFACES(QQmlParserStatus)
        Q_PROPERTY(QByteArray filterRoleName READ filterRoleName WRITE setFilterRoleName)
        Q_PROPERTY(QItemSelectionModel* selectionModel READ selectionModel CONSTANT)
        Q_PROPERTY(int selectedIndexesCount READ selectedIndexesCount NOTIFY selectionChanged)
    public:
        FilterProxyModel();
        void classBegin() override;
        void componentComplete() override;

        QByteArray filterRoleName() const;
        void setFilterRoleName(const QByteArray& filterRoleName);

        Q_INVOKABLE int count() const;
        Q_INVOKABLE int proxyIndex(int sourceIndex) const;
        Q_INVOKABLE int sourceIndex(int proxyIndex) const;

        QItemSelectionModel* selectionModel() const;
        int selectedIndexesCount() const;
        Q_INVOKABLE QList<int> selectedSourceIndexes() const;
        Q_INVOKABLE bool isSelected(int row) const;
        Q_INVOKABLE void select(int row);
        Q_INVOKABLE void selectAll();

    private:
        QByteArray mFilterRoleName;
        QItemSelectionModel* mSelectionModel;
    signals:
        void selectionChanged();
    };
}

#endif // UNPLAYER_FILTERPROXYMODEL_H
