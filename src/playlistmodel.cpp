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

#include "playlistmodel.h"

#include <algorithm>
#include <memory>
#include <set>
#include <unordered_map>

#include <QtConcurrentRun>

#include "modelutils.h"
#include "playlistutils.h"
#include "stdutils.h"
#include "tracksquery.h"
#include "utilsfunctions.h"

namespace unplayer
{
    namespace
    {
        const QLatin1String dbConnectionName("playlistmodel");
    }

    QVariant PlaylistModel::data(const QModelIndex& index, int role) const
    {
        const PlaylistTrack& track = mTracks[static_cast<size_t>(index.row())];
        switch (role) {
        case UrlRole:
            return track.url;
        case IsLocalFileRole:
            return track.url.isLocalFile();
        case FilePathRole:
            return track.url.path();
        case TitleRole:
            return track.title;
        case DurationRole:
            return track.duration;
        case ArtistRole:
            return track.artist;
        case AlbumRole:
            return track.album;
        default:
            return QVariant();
        }
    }

    int PlaylistModel::rowCount(const QModelIndex&) const
    {
        return static_cast<int>(mTracks.size());
    }

    bool PlaylistModel::removeRows(int row, int count, const QModelIndex& parent)
    {
        if (count > 0 && (row + count) <= static_cast<int>(mTracks.size())) {
            beginRemoveRows(parent, row, row + count - 1);
            const auto first(mTracks.begin() + row);
            mTracks.erase(first, first + count);
            endRemoveRows();
            return true;
        }
        return false;
    }

    QHash<int, QByteArray> PlaylistModel::roleNames() const
    {
        return {{UrlRole, "url"},
                {IsLocalFileRole, "isLocalFile"},
                {FilePathRole, "filePath"},
                {TitleRole, "title"},
                {DurationRole, "duration"},
                {ArtistRole, "artist"},
                {AlbumRole, "album"}};
    }

    const QString& PlaylistModel::filePath() const
    {
        return mFilePath;
    }

    void PlaylistModel::setFilePath(const QString& playlistFilePath)
    {
        if (playlistFilePath == mFilePath) {
            return;
        }

        mFilePath = playlistFilePath;

        setLoading(true);

        removeRows(0, rowCount());

        auto future = QtConcurrent::run([playlistFilePath]() {
            auto tracks(std::make_shared<std::vector<PlaylistTrack>>(PlaylistUtils::parsePlaylist(playlistFilePath)));

            std::set<QString> tracksToQuery;
            std::unordered_multimap<QString, PlaylistTrack*> tracksToQueryMap;
            tracksToQueryMap.reserve(tracks->size());
            for (PlaylistTrack& track : *tracks) {
                if (track.url.isLocalFile()) {
                    QString filePath(track.url.path());
                    tracksToQuery.insert(filePath);
                    tracksToQueryMap.insert({std::move(filePath), &track});
                }
            }

            queryTracksByPaths(std::move(tracksToQuery), tracksToQueryMap, dbConnectionName);

            return tracks;
        });

        onFutureFinished(future, this, [this](std::shared_ptr<std::vector<PlaylistTrack>>&& tracks) {
            removeRows(0, rowCount());

            beginInsertRows(QModelIndex(), 0, static_cast<int>(tracks->size() - 1));
            mTracks = std::move(*tracks);
            endInsertRows();

            setLoading(false);
        });
    }

    QStringList PlaylistModel::getTracks(const std::vector<int>& indexes)
    {
        QStringList tracks;
        tracks.reserve(static_cast<int>(mTracks.size()));
        for (int index : indexes) {
            tracks.push_back(mTracks[static_cast<size_t>(index)].url.toString());
        }
        return tracks;
    }

    void PlaylistModel::removeTrack(int index)
    {
        beginRemoveRows(QModelIndex(), index, index);
        mTracks.erase(mTracks.begin() + index);
        endRemoveRows();
        PlaylistUtils::instance()->savePlaylist(mFilePath, mTracks);
    }

    void PlaylistModel::removeTracks(const std::vector<int>& indexes)
    {
        ModelBatchRemover::removeIndexes(this, indexes);
        PlaylistUtils::instance()->savePlaylist(mFilePath, mTracks);
    }
}
