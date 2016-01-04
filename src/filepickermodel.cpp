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

#include "filepickermodel.h"
#include "filepickermodel.moc"

#include <QDir>

#include "utils.h"

namespace
{

enum FilePickerModelRole
{
    FilePathRole = Qt::UserRole,
    FileNameRole,
    IsDirectoryRole,
};

}

namespace Unplayer
{

struct FilePickerFile
{
    QString filePath;
    QString fileName;
    bool isDirectory;
};

FilePickerModel::~FilePickerModel()
{
    qDeleteAll(m_files);
}

void FilePickerModel::classBegin()
{

}

void FilePickerModel::componentComplete()
{
    setDirectory(Utils::homeDirectory());
}

QVariant FilePickerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const FilePickerFile *file = m_files.at(index.row());

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
    return m_files.size();
}

QString FilePickerModel::directory() const
{
    return m_directory;
}

void FilePickerModel::setDirectory(QString newDirectory)
{
    newDirectory = QDir(newDirectory).absolutePath();
    if (newDirectory != m_directory) {
        m_directory = newDirectory;
        emit directoryChanged();
        loadDirectory();
    }
}

QStringList FilePickerModel::nameFilters() const
{
    return m_nameFilters;
}

void FilePickerModel::setNameFilters(const QStringList &filters)
{
    m_nameFilters = filters;
}

QString FilePickerModel::parentDirectory() const
{
    return QFileInfo(m_directory).path();
}

QHash<int, QByteArray> FilePickerModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles.insert(FilePathRole, "filePath");
    roles.insert(FileNameRole, "fileName");
    roles.insert(IsDirectoryRole, "isDirectory");

    return roles;
}

void FilePickerModel::loadDirectory()
{
    beginResetModel();

    qDeleteAll(m_files);
    m_files.clear();

    for (const QFileInfo &info : QDir(m_directory).entryInfoList(m_nameFilters,
                                                                 QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot,
                                                                 QDir::DirsFirst)) {

        m_files.append(new FilePickerFile {
                           info.filePath(),
                           info.fileName(),
                           info.isDir()
                       });
    }

    endResetModel();
}

}
