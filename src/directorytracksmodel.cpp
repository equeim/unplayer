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

#include "directorytracksmodel.h"

#include <QDebug>
#include <QDir>
#include <QFutureWatcher>
#include <QItemSelectionModel>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QtConcurrentRun>

#include "libraryutils.h"
#include "playlistutils.h"
#include "settings.h"

namespace unplayer
{
    DirectoryTracksModel::DirectoryTracksModel()
        : mLoaded(false)
    {
    }

    void DirectoryTracksModel::classBegin()
    {
    }

    void DirectoryTracksModel::componentComplete()
    {
        setDirectory(Settings::instance()->defaultDirectory());
    }

    QVariant DirectoryTracksModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid()) {
            return QVariant();
        }

        const DirectoryTrackFile& file = mFiles.at(index.row());

        switch (role) {
        case FilePathRole:
            return file.filePath;
        case FileNameRole:
            return file.fileName;
        case IsDirectoryRole:
            return file.isDirectory;
        default:
            return QVariant();
        }
    }

    int DirectoryTracksModel::rowCount(const QModelIndex&) const
    {
        return mFiles.size();
    }

    const QVector<DirectoryTrackFile>& DirectoryTracksModel::files() const
    {
        return mFiles;
    }

    QString DirectoryTracksModel::directory() const
    {
        return mDirectory;
    }

    void DirectoryTracksModel::setDirectory(QString newDirectory)
    {
        QDir dir(newDirectory);
        if (!dir.isReadable()) {
            qWarning() << "directory is not readable:" << newDirectory;
            newDirectory = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        } else {
            newDirectory = dir.absolutePath();
        }
        if (newDirectory != mDirectory) {
            mDirectory = newDirectory;
            emit directoryChanged();
            loadDirectory();
        }
    }

    QString DirectoryTracksModel::parentDirectory() const
    {
        return QFileInfo(mDirectory).path();
    }

    bool DirectoryTracksModel::isLoaded() const
    {
        return mLoaded;
    }

    QString DirectoryTracksModel::getTrack(int index) const
    {
        return mFiles.at(index).filePath;
    }

    QStringList DirectoryTracksModel::getTracks(const QVector<int>& indexes) const
    {
        QStringList list;
        for (int index : indexes) {
            if (!mFiles.at(index).isDirectory) {
                list.append(getTrack(index));
            }
        }
        return list;
    }

    QHash<int, QByteArray> DirectoryTracksModel::roleNames() const
    {
        return {{FilePathRole, "filePath"},
                {FileNameRole, "fileName"},
                {IsDirectoryRole, "isDirectory"}};
    }

    void DirectoryTracksModel::loadDirectory()
    {
        mLoaded = false;
        emit loadedChanged();

        beginRemoveRows(QModelIndex(), 0, mFiles.size() - 1);
        mFiles.clear();
        endRemoveRows();

        const QString directory(mDirectory);
        auto future = QtConcurrent::run([directory]() {
            QVector<DirectoryTrackFile> files;
            const QMimeDatabase mimeDb;
            for (const QFileInfo& info : QDir(directory).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Files | QDir::Readable)) {
                if (info.isDir()) {
                    files.append({info.filePath(),
                                  info.fileName(),
                                  true});
                } else {
                    const QString mimeType(mimeDb.mimeTypeForFile(info, QMimeDatabase::MatchExtension).name());
                    if (LibraryUtils::supportedMimeTypes.contains(mimeType) || PlaylistUtils::playlistsMimeTypes.contains(mimeType)) {
                        files.append({info.filePath(),
                                      info.fileName(),
                                      false});
                    }
                }
            }
            return files;
        });

        using FutureWatcher = QFutureWatcher<QVector<DirectoryTrackFile>>;
        auto watcher = new FutureWatcher(this);
        QObject::connect(watcher, &FutureWatcher::finished, this, [=]() {
            const auto files = watcher->result();
            beginInsertRows(QModelIndex(), 0, files.size() - 1);
            mFiles = files;
            endInsertRows();
            mLoaded = true;
            emit loadedChanged();
        });
        watcher->setFuture(future);
    }

    DirectoryTracksProxyModel::DirectoryTracksProxyModel()
    {
        setFilterRole(DirectoryTracksModel::FileNameRole);
        setSortEnabled(true);
        setSortRole(DirectoryTracksModel::FileNameRole);
        setIsDirectoryRole(DirectoryTracksModel::IsDirectoryRole);
    }

    void DirectoryTracksProxyModel::componentComplete()
    {
        auto updateCount = [=]() {
            mDirectoriesCount = 0;
            mTracksCount = 0;
            const QVector<DirectoryTrackFile>& files = static_cast<const DirectoryTracksModel*>(sourceModel())->files();
            for (int i = 0, max = rowCount(); i < max; ++i) {
                if (files.at(sourceIndex(i)).isDirectory) {
                    mDirectoriesCount++;
                } else {
                    mTracksCount++;
                }
            }
            emit countChanged();
        };

        QObject::connect(this, &QAbstractItemModel::modelReset, this, updateCount);
        QObject::connect(this, &QAbstractItemModel::rowsInserted, this, updateCount);
        QObject::connect(this, &QAbstractItemModel::rowsRemoved, this, updateCount);

        updateCount();

        DirectoryContentProxyModel::componentComplete();
    }

    int DirectoryTracksProxyModel::directoriesCount() const
    {
        return mDirectoriesCount;
    }

    int DirectoryTracksProxyModel::tracksCount() const
    {
        return mTracksCount;
    }

    QVariantList DirectoryTracksProxyModel::getSelectedTracks() const
    {
        const QVector<int> indexes(selectedSourceIndexes());
        QVariantList tracks;
        tracks.reserve(indexes.size());
        auto model = static_cast<const DirectoryTracksModel*>(sourceModel());
        for (int index : indexes) {
            tracks.append(model->getTrack(index));
        }
        return tracks;
    }

    void DirectoryTracksProxyModel::selectAll()
    {
        const QVector<DirectoryTrackFile>& files = static_cast<const DirectoryTracksModel*>(sourceModel())->files();
        for (int i = 0, max = rowCount(); i < max; ++i) {
            if (!files.at(sourceIndex(i)).isDirectory) {
                selectionModel()->select(index(i, 0), QItemSelectionModel::Select);
            }
        }
    }
}
