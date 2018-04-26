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

#ifndef UNPLAYER_FILTERPROXYMODEL_H
#define UNPLAYER_FILTERPROXYMODEL_H

#include <QCollator>
#include <QQmlParserStatus>
#include <QSortFilterProxyModel>

class QItemSelectionModel;

namespace unplayer
{
    class FilterProxyModel : public QSortFilterProxyModel, public QQmlParserStatus
    {
        Q_OBJECT
        Q_INTERFACES(QQmlParserStatus)
        Q_PROPERTY(bool sortEnabled READ isSortEnabled WRITE setSortEnabled)
        Q_PROPERTY(QVector<int> sourceIndexes READ sourceIndexes)
        Q_PROPERTY(QItemSelectionModel* selectionModel READ selectionModel CONSTANT)
        Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
        Q_PROPERTY(int selectedIndexesCount READ selectedIndexesCount NOTIFY selectionChanged)
        Q_PROPERTY(QVector<int> selectedSourceIndexes READ selectedSourceIndexes)
    public:
        FilterProxyModel();
        void classBegin() override;
        void componentComplete() override;

        QVector<int> sourceIndexes() const;

        bool isSortEnabled() const;
        void setSortEnabled(bool sortEnabled);

        Q_INVOKABLE int proxyIndex(int sourceIndex) const;
        Q_INVOKABLE int sourceIndex(int proxyIndex) const;

        QItemSelectionModel* selectionModel() const;
        bool hasSelection() const;
        int selectedIndexesCount() const;
        QVector<int> selectedSourceIndexes() const;
        Q_INVOKABLE bool isSelected(int row) const;
        Q_INVOKABLE void select(int row);
        Q_INVOKABLE void selectAll();

    protected:
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

    private:
        QCollator mCollator;
        bool mSortEnabled;
        QItemSelectionModel* mSelectionModel;
    signals:
        void selectionChanged();
    };
}

#endif // UNPLAYER_FILTERPROXYMODEL_H
