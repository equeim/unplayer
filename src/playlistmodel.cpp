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

#include "playlistmodel.h"

#include <algorithm>

#include "playlistutils.h"

namespace unplayer
{
    PlaylistModel::PlaylistModel()
    {
    }

    QVariant PlaylistModel::data(const QModelIndex& index, int role) const
    {
        const PlaylistTrack& track = mTracks.at(index.row());
        switch (role) {
        case FilePathRole:
            return track.filePath;
        case TitleRole:
            return track.title;
        case DurationRole:
            return track.duration;
        case HasDurationRole:
            return track.hasDuration;
        case ArtistRole:
            return track.artist;
        case AlbumRole:
            return track.album;
        case InLibraryRole:
            return track.inLibrary;
        default:
            return QVariant();
        }
    }

    int PlaylistModel::rowCount(const QModelIndex&) const
    {
        return mTracks.size();
    }

    QHash<int, QByteArray> PlaylistModel::roleNames() const
    {
        return {{FilePathRole, "filePath"},
                {TitleRole, "title"},
                {DurationRole, "duration"},
                {HasDurationRole, "hasDuration"},
                {ArtistRole, "artist"},
                {AlbumRole, "album"},
                {InLibraryRole, "inLibrary"}};
    }

    const QString& PlaylistModel::filePath() const
    {
        return mFilePath;
    }

    void PlaylistModel::setFilePath(const QString& filePath)
    {
        mFilePath = filePath;
        mTracks = PlaylistUtils::parsePlaylist(mFilePath);
    }

    QStringList PlaylistModel::getTracks(const QVector<int>& indexes)
    {
        QStringList tracks;
        tracks.reserve(mTracks.size());
        for (int index : indexes) {
            tracks.append(mTracks.at(index).filePath);
        }
        return tracks;
    }

    void PlaylistModel::removeTrack(int index)
    {
        beginRemoveRows(QModelIndex(), index, index);
        mTracks.removeAt(index);
        endRemoveRows();
        PlaylistUtils::instance()->savePlaylist(mFilePath, mTracks);
    }

    void PlaylistModel::removeTracks(QVector<int> indexes)
    {
        std::reverse(indexes.begin(), indexes.end());
        for (int index : indexes) {
            beginRemoveRows(QModelIndex(), index, index);
            mTracks.removeAt(index);
            endRemoveRows();
        }
        PlaylistUtils::instance()->savePlaylist(mFilePath, mTracks);
    }
}
