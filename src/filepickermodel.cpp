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

#include "filepickermodel.h"

#include <QDir>

#include "utils.h"

namespace unplayer
{
    namespace
    {
        enum FilePickerModelRole
        {
            FilePathRole = Qt::UserRole,
            FileNameRole,
            IsDirectoryRole,
        };
    }

    struct FilePickerFile
    {
        QString filePath;
        QString fileName;
        bool isDirectory;
    };

    FilePickerModel::~FilePickerModel()
    {
        qDeleteAll(mFiles);
    }

    void FilePickerModel::classBegin()
    {
    }

    void FilePickerModel::componentComplete()
    {
        setDirectory(Utils::homeDirectory());
    }

    QVariant FilePickerModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid()) {
            return QVariant();
        }

        const FilePickerFile* file = mFiles.at(index.row());

        switch (role) {
        case FilePathRole:
            return file->filePath;
        case FileNameRole:
            return file->fileName;
        case IsDirectoryRole:
            return file->isDirectory;
        default:
            return QVariant();
        }
    }

    int FilePickerModel::rowCount(const QModelIndex&) const
    {
        return mFiles.size();
    }

    QString FilePickerModel::directory() const
    {
        return mDirectory;
    }

    void FilePickerModel::setDirectory(QString newDirectory)
    {
        newDirectory = QDir(newDirectory).absolutePath();
        if (newDirectory != mDirectory) {
            mDirectory = newDirectory;
            emit directoryChanged();
            loadDirectory();
        }
    }

    QStringList FilePickerModel::nameFilters() const
    {
        return mNameFilters;
    }

    void FilePickerModel::setNameFilters(const QStringList& filters)
    {
        mNameFilters = filters;
    }

    QString FilePickerModel::parentDirectory() const
    {
        return QFileInfo(mDirectory).path();
    }

    QHash<int, QByteArray> FilePickerModel::roleNames() const
    {
        return {{FilePathRole, QByteArrayLiteral("filePath")},
                {FileNameRole, QByteArrayLiteral("fileName")},
                {IsDirectoryRole, QByteArrayLiteral("isDirectory")}};
    }

    void FilePickerModel::loadDirectory()
    {
        beginResetModel();

        qDeleteAll(mFiles);
        mFiles.clear();

        for (const QFileInfo& info : QDir(mDirectory).entryInfoList(mNameFilters, QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot, QDir::DirsFirst)) {

            mFiles.append(new FilePickerFile{
                info.filePath(),
                info.fileName(),
                info.isDir()});
        }

        endResetModel();
    }
}
