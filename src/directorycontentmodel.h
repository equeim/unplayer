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

#ifndef UNPLAYER_DIRECTORYCONTENTMODEL_H
#define UNPLAYER_DIRECTORYCONTENTMODEL_H

#include <QAbstractListModel>
#include <QQmlParserStatus>

namespace unplayer
{
    struct DirectoryContentFile
    {
        QString filePath;
        QString name;
        bool isDirectory;
    };

    class DirectoryContentModel : public QAbstractListModel, public QQmlParserStatus
    {
        Q_OBJECT
        Q_INTERFACES(QQmlParserStatus)
        Q_ENUMS(Role)
        Q_PROPERTY(QString directory READ directory WRITE setDirectory NOTIFY directoryChanged)
        Q_PROPERTY(QString parentDirectory READ parentDirectory NOTIFY directoryChanged)
        Q_PROPERTY(bool showFiles READ showFiles WRITE setShowFiles)
        Q_PROPERTY(QStringList nameFilters READ nameFilters WRITE setNameFilters)
        Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    public:
        enum Role
        {
            FilePathRole,
            NameRole,
            IsDirectoryRole
        };

        DirectoryContentModel();
        void classBegin() override;
        void componentComplete() override;

        QVariant data(const QModelIndex& index, int role) const override;
        int rowCount(const QModelIndex&) const;

        const QString& directory() const;
        void setDirectory(const QString& directory);

        const QString& parentDirectory() const;

        bool showFiles() const;
        void setShowFiles(bool show);

        const QStringList& nameFilters() const;
        void setNameFilters(const QStringList& filters);

        bool isLoading() const;

    protected:
        QHash<int, QByteArray> roleNames() const override;

    private:
        void loadDirectory();

        bool mComponentCompleted;
        QVector<DirectoryContentFile> mFiles;
        QString mDirectory;
        QString mParentDirectory;
        bool mShowFiles;
        QStringList mNameFilters;

        bool mLoading;
    signals:
        void directoryChanged();
        void loadingChanged();
    };
}

Q_DECLARE_TYPEINFO(unplayer::DirectoryContentFile, Q_MOVABLE_TYPE);

#endif // UNPLAYER_DIRECTORYCONTENTMODEL_H
