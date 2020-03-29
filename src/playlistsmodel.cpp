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

#include "playlistsmodel.h"

#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QtConcurrentRun>

#include "modelutils.h"
#include "playlistutils.h"
#include "stdutils.h"

namespace unplayer
{
    bool PlaylistsModelItem::operator==(const PlaylistsModelItem& other) const
    {
        return (other.filePath == filePath);
    }

    PlaylistsModel::PlaylistsModel()
    {
        update();
        QObject::connect(PlaylistUtils::instance(), &PlaylistUtils::playlistsChanged, this, &PlaylistsModel::update);
    }

    QVariant PlaylistsModel::data(const QModelIndex& index, int role) const
    {
        const PlaylistsModelItem& playlist = mPlaylists[static_cast<size_t>(index.row())];
        switch (role) {
        case FilePathRole:
            return playlist.filePath;
        case NameRole:
            return playlist.name;
        case TracksCountRole:
            return playlist.tracksCount;
        default:
            return QVariant();
        }
    }

    int PlaylistsModel::rowCount(const QModelIndex&) const
    {
        return static_cast<int>(mPlaylists.size());
    }

    bool PlaylistsModel::removeRows(int row, int count, const QModelIndex& parent)
    {
        beginRemoveRows(parent, row, row + count - 1);
        const auto first(mPlaylists.begin() + row);
        mPlaylists.erase(first, first + count);
        endRemoveRows();
        return true;
    }

    void PlaylistsModel::removePlaylists(const std::vector<int>& indexes) const
    {
        std::vector<QString> paths;
        paths.reserve(indexes.size());
        for (int index : indexes) {
            paths.push_back(mPlaylists[static_cast<size_t>(index)].filePath);
        }
        PlaylistUtils::instance()->removePlaylists(paths);
    }

    QStringList PlaylistsModel::getTracksForPlaylists(const std::vector<int>& indexes) const
    {
        QStringList tracks;
        for (int index : indexes) {
            tracks.append(PlaylistUtils::getPlaylistTracks(mPlaylists[static_cast<size_t>(index)].filePath));
        }
        return tracks;
    }

    QHash<int, QByteArray> PlaylistsModel::roleNames() const
    {
        return {{FilePathRole, "filePath"},
                {NameRole, "name"},
                {TracksCountRole, "tracksCount"}};
    }

    void PlaylistsModel::update()
    {
        auto future = QtConcurrent::run([]() {
            std::vector<PlaylistsModelItem> playlists;
            const QList<QFileInfo> files(QDir(PlaylistUtils::instance()->playlistsDirectoryPath())
                                         .entryInfoList(PlaylistUtils::playlistsNameFilters(), QDir::Files));
            playlists.reserve(static_cast<size_t>(files.size()));
            for (const QFileInfo& fileInfo : files) {
                playlists.push_back(PlaylistsModelItem{fileInfo.filePath(),
                                                       fileInfo.completeBaseName(),
                                                       PlaylistUtils::getPlaylistTracksCount(fileInfo.filePath())});
            }
            return playlists;
        });

        using FutureWatcher = QFutureWatcher<std::vector<PlaylistsModelItem>>;
        auto watcher = new FutureWatcher(this);
        QObject::connect(watcher, &FutureWatcher::finished, this, [=]() {
            std::vector<PlaylistsModelItem> playlists(watcher->result());
            ModelBatchRemover remover(this);
            for (int i = static_cast<int>(mPlaylists.size()) - 1; i >= 0; --i) {
                if (!contains(playlists, mPlaylists[static_cast<size_t>(i)])) {
                    remover.remove(i);
                }
            }
            remover.remove();

            bool inserting = false;
            const size_t oldSize = mPlaylists.size();
            if (playlists.size() > oldSize) {
                inserting = true;
                mPlaylists.reserve(playlists.size());
                beginInsertRows(QModelIndex(), static_cast<int>(oldSize), static_cast<int>(playlists.size()) - 1);
            }
            for (PlaylistsModelItem& playlist : playlists) {
                auto found(std::find(mPlaylists.begin(), mPlaylists.end(), playlist));
                if (found == mPlaylists.cend()) {
                    mPlaylists.push_back(std::move(playlist));
                } else {
                    *found = std::move(playlist);
                }
            }

            if (inserting) {
                endInsertRows();
            }

            emit dataChanged(index(0), index(static_cast<int>(oldSize) - 1));
        });
        watcher->setFuture(future);
    }
}
