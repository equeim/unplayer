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

#include <memory>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QItemSelectionModel>
#include <QStandardPaths>
#include <QtConcurrentRun>

#include "fileutils.h"
#include "libraryutils.h"
#include "modelutils.h"
#include "playlistutils.h"
#include "settings.h"
#include "utilsfunctions.h"

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

        const DirectoryTrackFile& file = mFiles[static_cast<size_t>(index.row())];

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
        return static_cast<int>(mFiles.size());
    }

    bool DirectoryTracksModel::removeRows(int row, int count, const QModelIndex& parent)
    {
        if (count > 0 && (row + count) <= static_cast<int>(mFiles.size())) {
            beginRemoveRows(parent, row, row + count - 1);
            const auto first(mFiles.begin() + row);
            mFiles.erase(first, first + count);
            endRemoveRows();
            return true;
        }
        return false;
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

    QString DirectoryTracksModel::getTrack(int index) const
    {
        return mFiles[static_cast<size_t>(index)].filePath;
    }

    QStringList DirectoryTracksModel::getTracks(const std::vector<int>& indexes, bool includePlaylists) const
    {
        QStringList tracks;
        tracks.reserve(mTracksCount);
        for (int index : indexes) {
            const DirectoryTrackFile& file = mFiles[static_cast<size_t>(index)];
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

    void DirectoryTracksModel::removeTracks(const std::vector<int>& indexes)
    {
        if (LibraryUtils::instance()->isRemovingFiles() || isLoading()) {
            return;
        }

        std::vector<QString> files;
        files.reserve(indexes.size());
        for (int index : indexes) {
            files.push_back(mFiles[static_cast<size_t>(index)].filePath);
        }
        LibraryUtils::instance()->removeTracksByPaths(std::move(files), true, true);
        QObject::connect(LibraryUtils::instance(), &LibraryUtils::removingFilesChanged, this, [this, indexes] {
            if (!LibraryUtils::instance()->isRemovingFiles()) {
                ModelBatchRemover::removeIndexes(this, indexes);
                QObject::disconnect(LibraryUtils::instance(), &LibraryUtils::removingFilesChanged, this, nullptr);
            }
        });
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

        setLoading(true);

        removeRows(0, rowCount());

        struct FutureResult
        {
            std::vector<DirectoryTrackFile> files;
            int tracksCount;
        };
        auto future = QtConcurrent::run([directory = mDirectory, showVideoFiles = mShowVideoFiles]() {
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
                            fileutils::isExtensionSupported(suffix) ||
                            (showVideoFiles && fileutils::isVideoExtensionSupported(suffix))) {
                        files.push_back({info.filePath(),
                                         info.fileName(),
                                         false,
                                         isPlaylist});
                        ++tracksCount;
                    }
                }
            }
            return std::make_shared<FutureResult>(FutureResult{std::move(files), tracksCount});
        });

        onFutureFinished(future, this, [this](std::shared_ptr<FutureResult>&& result) {
            removeRows(0, rowCount());

            beginInsertRows(QModelIndex(), 0, static_cast<int>(result->files.size()) - 1);
            mFiles = std::move(result->files);
            endInsertRows();
            mTracksCount = result->tracksCount;

            setLoading(false);
        });
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
                if (files[static_cast<size_t>(sourceIndex(i))].isDirectory) {
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
        tracks.reserve(static_cast<int>(indexes.size()));
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
            if (!files[static_cast<size_t>(sourceIndex(i))].isDirectory) {
                firstFileIndex = i;
                break;
            }
        }
        if (firstFileIndex != -1) {
            selectionModel()->select(QItemSelection(index(firstFileIndex, 0), index(rowCount() - 1, 0)), QItemSelectionModel::Select);
        }
    }
}
