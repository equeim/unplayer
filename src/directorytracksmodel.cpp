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

#include "directorytracksmodel.h"

#include <functional>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFutureWatcher>
#include <QItemSelectionModel>
#include <QStandardPaths>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtConcurrentRun>

#include "libraryutils.h"
#include "playlistutils.h"
#include "settings.h"

namespace unplayer
{
    void DirectoryTracksModel::classBegin()
    {
    }

    void DirectoryTracksModel::componentComplete()
    {
        mShowVideoFiles = Settings::instance()->showVideoFiles();
        setDirectory(Settings::instance()->defaultDirectory());
    }

    QVariant DirectoryTracksModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid()) {
            return QVariant();
        }

        const DirectoryTrackFile& file = mFiles[index.row()];

        switch (role) {
        case FilePathRole:
            return file.filePath;
        case FileNameRole:
            return file.fileName;
        case IsDirectoryRole:
            return file.isDirectory;
        case IsPlaylistRole:
            return file.isPlaylist;
        default:
            return QVariant();
        }
    }

    int DirectoryTracksModel::rowCount(const QModelIndex&) const
    {
        return mFiles.size();
    }

    const std::vector<DirectoryTrackFile>& DirectoryTracksModel::files() const
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
        return mFiles[index].filePath;
    }

    QStringList DirectoryTracksModel::getTracks(const std::vector<int>& indexes, bool includePlaylists) const
    {
        QStringList tracks;
        tracks.reserve(mTracksCount);
        for (int index : indexes) {
            const DirectoryTrackFile& file = mFiles[index];
            if (!file.isDirectory && (!file.isPlaylist || includePlaylists)) {
                tracks.push_back(getTrack(index));
            }
        }
        return tracks;
    }

    void DirectoryTracksModel::removeTrack(int index)
    {
        removeTracks({index});
    }

    void DirectoryTracksModel::removeTracks(std::vector<int> indexes)
    {
        if (LibraryUtils::instance()->isRemovingFiles() || !mLoaded) {
            return;
        }

        std::vector<QString> files;
        files.reserve(indexes.size());
        for (int index : indexes) {
            files.push_back(mFiles[index].filePath);
        }
        QObject::connect(LibraryUtils::instance(), &LibraryUtils::removingFilesChanged, this, std::bind([this](std::vector<int>& indexes) {
            if (!LibraryUtils::instance()->isRemovingFiles()) {
                for (int i = indexes.size() - 1; i >= 0; --i) {
                    const int index = indexes[i];
                    beginRemoveRows(QModelIndex(), index, index);
                    mFiles.erase(mFiles.begin() + index);
                    endRemoveRows();
                }
                QObject::disconnect(LibraryUtils::instance(), &LibraryUtils::removingFilesChanged, this, nullptr);
            }
        }, std::move(indexes)));
        LibraryUtils::instance()->removeFiles(std::move(files), true, true);
    }

    QHash<int, QByteArray> DirectoryTracksModel::roleNames() const
    {
        return {{FilePathRole, "filePath"},
                {FileNameRole, "fileName"},
                {IsDirectoryRole, "isDirectory"},
                {IsPlaylistRole, "isPlaylist"}};
    }

    void DirectoryTracksModel::loadDirectory()
    {
        if (LibraryUtils::instance()->isRemovingFiles()) {
            QObject::disconnect(LibraryUtils::instance(), &LibraryUtils::removingFilesChanged, this, nullptr);
            return;
        }

        mLoaded = false;
        emit loadedChanged();

        beginRemoveRows(QModelIndex(), 0, mFiles.size() - 1);
        mFiles.clear();
        endRemoveRows();

        const QString directory(mDirectory);
        const bool showVideoFiles = mShowVideoFiles;
        auto future = QtConcurrent::run([directory, showVideoFiles]() {
            std::vector<DirectoryTrackFile> files;
            int tracksCount = 0;
            const QFileInfoList fileInfos(QDir(directory).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Files | QDir::Readable));
            for (const QFileInfo& info : fileInfos) {
                if (info.isDir()) {
                    files.push_back({info.filePath(),
                                     info.fileName(),
                                     true,
                                     false});
                } else {
                    const QString suffix(info.suffix());
                    const bool isPlaylist = PlaylistUtils::isPlaylistExtension(suffix);
                    if (isPlaylist ||
                            LibraryUtils::isExtensionSupported(suffix) ||
                            (showVideoFiles && LibraryUtils::isVideoExtensionSupported(suffix))) {
                        files.push_back({info.filePath(),
                                         info.fileName(),
                                         false,
                                         isPlaylist});
                        ++tracksCount;
                    }
                }
            }
            return std::pair<std::vector<DirectoryTrackFile>, int>(std::move(files), tracksCount);
        });

        using FutureWatcher = QFutureWatcher<std::pair<std::vector<DirectoryTrackFile>, int>>;
        auto watcher = new FutureWatcher(this);
        QObject::connect(watcher, &FutureWatcher::finished, this, [=]() {
            auto result(watcher->result());
            beginInsertRows(QModelIndex(), 0, result.first.size() - 1);
            mFiles = std::move(result.first);
            endInsertRows();
            mTracksCount = result.second;

            mLoaded = true;
            emit loadedChanged();
        });
        watcher->setFuture(future);
    }

    DirectoryTracksProxyModel::DirectoryTracksProxyModel()
        : mDirectoriesCount(0),
          mTracksCount(0)
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
            const std::vector<DirectoryTrackFile>& files = static_cast<const DirectoryTracksModel*>(sourceModel())->files();
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

    QStringList DirectoryTracksProxyModel::getSelectedTracks() const
    {
        const std::vector<int> indexes(selectedSourceIndexes());
        QStringList tracks;
        tracks.reserve(indexes.size());
        auto model = static_cast<const DirectoryTracksModel*>(sourceModel());
        for (int index : indexes) {
            tracks.push_back(model->getTrack(index));
        }
        return tracks;
    }

    void DirectoryTracksProxyModel::selectAll()
    {
        const std::vector<DirectoryTrackFile>& files = static_cast<const DirectoryTracksModel*>(sourceModel())->files();
        int firstFileIndex = -1;
        for (int i = 0, max = rowCount(); i < max; ++i) {
            if (!files[sourceIndex(i)].isDirectory) {
                firstFileIndex = i;
                break;
            }
        }
        if (firstFileIndex != -1) {
            selectionModel()->select(QItemSelection(index(firstFileIndex, 0), index(rowCount() - 1, 0)), QItemSelectionModel::Select);
        }
    }
}
