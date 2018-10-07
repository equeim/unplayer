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
#include <unordered_map>

#include <QFileInfo>
#include <QSqlQuery>

#include "playlistutils.h"
#include "stdutils.h"

namespace unplayer
{
    QVariant PlaylistModel::data(const QModelIndex& index, int role) const
    {
        const PlaylistTrack& track = mTracks[index.row()];
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
        return mTracks.size();
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

    void PlaylistModel::setFilePath(const QString& filePath)
    {
        mFilePath = filePath;
        mTracks = PlaylistUtils::parsePlaylist(mFilePath);

        std::vector<QString> tracksToQuery;
        tracksToQuery.reserve(mTracks.size());
        std::unordered_map<QString, PlaylistTrack*> tracksMap;
        tracksMap.reserve(mTracks.size());
        for (PlaylistTrack& track : mTracks) {
            if (track.url.isLocalFile()) {
                QString filePath(track.url.path());
                tracksToQuery.push_back(filePath);
                tracksMap.insert({std::move(filePath), &track});
            }

            if (track.title.isEmpty()) {
                if (track.url.isLocalFile()) {
                    track.title = QFileInfo(track.url.path()).fileName();
                } else {
                    track.title = track.url.toString();
                }
            }
        }

        {
            /*auto db = QSqlDatabase::addDatabase(LibraryUtils::databaseType, dbConnectionName);
            db.setDatabaseName(LibraryUtils::instance()->databaseFilePath());
            if (!db.open()) {
                qWarning() << "failed to open database" << db.lastError();
            }
            db.transaction();*/

            const int maxParametersCount = 999;
            for (int i = 0, max = tracksToQuery.size(); i < max; i += maxParametersCount) {
                const int count = [&]() -> int {
                    const int left = max - i;
                    if (left > maxParametersCount) {
                        return maxParametersCount;
                    }
                    return left;
                }();

                QString queryString(QLatin1String("SELECT filePath, title, artist, album, duration FROM tracks WHERE filePath IN (?"));
                queryString.reserve(queryString.size() + (count - 1) * 2 + 1);
                for (int j = 1; j < count; ++j) {
                    queryString.push_back(QStringLiteral(",?"));
                }
                queryString.push_back(QLatin1Char(')'));

                //QSqlQuery query(db);
                QSqlQuery query;
                query.prepare(queryString);
                for (int j = i, max = i + count; j < max; ++j) {
                    query.addBindValue(tracksToQuery[j]);
                }

                if (query.exec()) {
                    QString previousFilePath;
                    QStringList artists;
                    QStringList albums;

                    const auto tracksMapEnd(tracksMap.end());
                    const auto fill = [&]() {
                        const auto found(tracksMap.find(previousFilePath));
                        if (found != tracksMapEnd) {
                            PlaylistTrack* track = found->second;
                            track->title = query.value(1).toString();
                            artists.removeDuplicates();
                            track->artist = artists.join(QStringLiteral(", "));
                            albums.removeDuplicates();
                            track->album = albums.join(QStringLiteral(", "));
                            track->duration = query.value(4).toInt();
                        }
                    };


                    while (query.next()) {
                        QString filePath(query.value(0).toString());

                        if (filePath != previousFilePath) {
                            if (!previousFilePath.isEmpty()) {
                                fill();
                            }
                        }

                        artists.push_back(query.value(2).toString());
                        albums.push_back(query.value(3).toString());

                        previousFilePath = std::move(filePath);
                    }

                    if (query.previous()) {
                        fill();
                    }
                }
            }

            //db.commit();
        }
        //QSqlDatabase::removeDatabase(dbConnectionName);
    }

    QStringList PlaylistModel::getTracks(const std::vector<int>& indexes)
    {
        QStringList tracks;
        tracks.reserve(mTracks.size());
        for (int index : indexes) {
            tracks.push_back(mTracks[index].url.toString());
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

    void PlaylistModel::removeTracks(std::vector<int> indexes)
    {
        std::sort(indexes.begin(), indexes.end(), std::greater<int>());
        for (int index : indexes) {
            beginRemoveRows(QModelIndex(), index, index);
            mTracks.erase(mTracks.begin() + index);
            endRemoveRows();
        }
        PlaylistUtils::instance()->savePlaylist(mFilePath, mTracks);
    }
}
