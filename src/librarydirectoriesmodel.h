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

#ifndef UNPLAYER_LIBRARYDIRECTORIESMODEL_H
#define UNPLAYER_LIBRARYDIRECTORIESMODEL_H

#include <vector>
#include <QAbstractListModel>
#include <QQmlParserStatus>

namespace unplayer
{
    class LibraryDirectoriesModel : public QAbstractListModel, public QQmlParserStatus
    {
        Q_OBJECT
        Q_INTERFACES(QQmlParserStatus)
        Q_PROPERTY(Type type READ type WRITE setType)
    public:
        enum Type
        {
            Library,
            Blacklisted
        };
        Q_ENUM(Type)

        void classBegin() override;
        void componentComplete() override;

        QVariant data(const QModelIndex& index, int role) const override;
        int rowCount(const QModelIndex& parent) const override;
        bool removeRows(int row, int count, const QModelIndex& parent) override;

        Type type() const;
        void setType(Type type);

        Q_INVOKABLE void addDirectory(const QString& directory);
        Q_INVOKABLE void removeDirectory(int index);
        Q_INVOKABLE void removeDirectories(const std::vector<int>& indexes);

    protected:
        QHash<int, QByteArray> roleNames() const override;
    private:
        QStringList mDirectories;
        Type mType = Library;
    };
}

#endif // UNPLAYER_LIBRARYDIRECTORIESMODEL_H
