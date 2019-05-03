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

#include "playlistsmodel.h"

#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QtConcurrentRun>

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
        const PlaylistsModelItem& playlist = mPlaylists.at(index.row());
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
        return mPlaylists.size();
    }

    void PlaylistsModel::removePlaylists(const std::vector<int>& indexes) const
    {
        std::vector<QString> paths;
        paths.reserve(indexes.size());
        for (int index : indexes) {
            paths.push_back(mPlaylists[index].filePath);
        }
        PlaylistUtils::instance()->removePlaylists(paths);
    }

    QStringList PlaylistsModel::getTracksForPlaylists(const std::vector<int>& indexes) const
    {
        QStringList tracks;
        for (int index : indexes) {
            tracks.append(PlaylistUtils::getPlaylistTracks(mPlaylists[index].filePath));
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
            playlists.reserve(files.size());
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

            for (int i = 0, max = mPlaylists.size(); i < max; ++i) {
                if (!contains(playlists, mPlaylists[i])) {
                    beginRemoveRows(QModelIndex(), i, i);
                    mPlaylists.erase(mPlaylists.begin() + i);
                    endRemoveRows();
                    i--;
                    max--;
                }
            }

            for (PlaylistsModelItem& playlist : playlists) {
                auto found(std::find(mPlaylists.begin(), mPlaylists.end(), playlist));
                if (found == mPlaylists.cend()) {
                    beginInsertRows(QModelIndex(), mPlaylists.size(), mPlaylists.size());
                    mPlaylists.push_back(std::move(playlist));
                    endInsertRows();
                } else {
                    *found = std::move(playlist);
                }
            }

            emit dataChanged(index(0), index(mPlaylists.size() - 1));
        });
        watcher->setFuture(future);
    }
}
