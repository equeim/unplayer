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

#include "playlistsmodel.h"

#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QSettings>
#include <QtConcurrentRun>

#include "playlistutils.h"

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

    void PlaylistsModel::removePlaylists(const QVector<int>& indexes) const
    {
        QStringList paths;
        paths.reserve(indexes.size());
        for (int index : indexes) {
            paths.append(mPlaylists.at(index).filePath);
        }
        PlaylistUtils::instance()->removePlaylists(paths);
    }

    QStringList PlaylistsModel::getTracksForPlaylists(const QVector<int>& indexes) const
    {
        QStringList tracks;
        for (int index : indexes) {
            tracks.append(PlaylistUtils::getPlaylistTracks(mPlaylists.at(index).filePath));
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
            QList<PlaylistsModelItem> playlists;
            QDir dir(PlaylistUtils::playlistsDirectoryPath());
            for (const QFileInfo& fileInfo : dir.entryInfoList({QLatin1String("*.pls")}, QDir::Files)) {
                playlists.append(PlaylistsModelItem{fileInfo.filePath(),
                                                    fileInfo.completeBaseName(),
                                                    PlaylistUtils::getPlaylistTracksCount(fileInfo.filePath())});
            }
            return playlists;
        });

        using FutureWatcher = QFutureWatcher<QList<PlaylistsModelItem>>;
        auto watcher = new FutureWatcher(this);
        QObject::connect(watcher, &FutureWatcher::finished, this, [=]() {
            const QList<PlaylistsModelItem> playlists(watcher->result());

            for (int i = 0, max = mPlaylists.size(); i < max; ++i) {
                if (!playlists.contains(mPlaylists.at(i))) {
                    beginRemoveRows(QModelIndex(), i, i);
                    mPlaylists.removeAt(i);
                    endRemoveRows();
                    i--;
                    max--;
                }
            }

            for (const PlaylistsModelItem& playlist : playlists) {
                const int index = mPlaylists.indexOf(playlist);
                if (index == -1) {
                    beginInsertRows(QModelIndex(), mPlaylists.size(), mPlaylists.size());
                    mPlaylists.append(playlist);
                    endInsertRows();
                } else {
                    mPlaylists[index] = playlist;
                }
            }

            emit dataChanged(index(0), index(mPlaylists.size() - 1));
        });
        watcher->setFuture(future);
    }
}
