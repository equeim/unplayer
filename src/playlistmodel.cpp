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
#include <unordered_map>

#include <QFileInfo>
#include <QFutureWatcher>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtConcurrentRun>

#include "libraryutils.h"
#include "modelutils.h"
#include "playlistutils.h"
#include "stdutils.h"
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

    bool PlaylistModel::isLoaded() const
    {
        return mLoaded;
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
        mFilePath = playlistFilePath;

        if (mLoaded) {
            mLoaded = false;
            emit loadedChanged();
        }

        auto future = QtConcurrent::run([playlistFilePath]() {
            std::vector<PlaylistTrack> tracks(PlaylistUtils::parsePlaylist(playlistFilePath));

            std::vector<QString> tracksToQuery;
            tracksToQuery.reserve(tracks.size());
            std::unordered_map<QString, PlaylistTrack*> tracksMap;
            tracksMap.reserve(tracks.size());
            for (PlaylistTrack& track : tracks) {
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

            const DatabaseGuard databaseGuard{dbConnectionName};

            QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
            if (!db.isOpen()) {
                return tracks;
            }

            db.transaction();
            const CommitGuard commitGuard{db};

            batchedCount(tracksToQuery.size(), LibraryUtils::maxDbVariableCount, [&](size_t first, size_t count) {
                QString queryString(QLatin1String("SELECT filePath, title, artist, album, duration FROM tracks WHERE filePath IN (?"));
                queryString.reserve(queryString.size() + static_cast<int>(count - 1) * 2 + 1);
                for (size_t j = 1; j < count; ++j) {
                    queryString.push_back(QStringLiteral(",?"));
                }
                queryString.push_back(QLatin1Char(')'));

                QSqlQuery query(db);
                query.prepare(queryString);
                for (size_t j = first, max = first + count; j < max; ++j) {
                    query.addBindValue(tracksToQuery[j]);
                }

                if (query.exec()) {
                    QString previousFilePath;
                    QString title;
                    QStringList artists;
                    QStringList albums;
                    int duration = 0;

                    const auto tracksMapEnd(tracksMap.end());
                    const auto fill = [&]() {
                        const auto found(tracksMap.find(previousFilePath));
                        if (found != tracksMapEnd) {
                            PlaylistTrack* track = found->second;
                            track->title = std::move(title);
                            artists.removeDuplicates();
                            track->artist = artists.join(QStringLiteral(", "));
                            albums.removeDuplicates();
                            track->album = albums.join(QStringLiteral(", "));
                            track->duration = duration;
                        }
                    };


                    while (query.next()) {
                        QString filePath(query.value(0).toString());

                        if (filePath != previousFilePath) {
                            if (!previousFilePath.isEmpty()) {
                                fill();
                            }

                            title = query.value(1).toString();
                            artists.clear();
                            albums.clear();
                            duration = query.value(4).toInt();
                        }

                        artists.push_back(query.value(2).toString());
                        albums.push_back(query.value(3).toString());

                        previousFilePath = std::move(filePath);
                    }

                    if (query.previous()) {
                        fill();
                    }
                }
            });

            return tracks;
        });

        using FutureWatcher = QFutureWatcher<std::vector<PlaylistTrack>>;
        auto watcher = new FutureWatcher(this);
        QObject::connect(watcher, &FutureWatcher::finished, this, [=]() {
            auto tracks(watcher->result());
            beginInsertRows(QModelIndex(), 0, static_cast<int>(tracks.size() - 1));
            mTracks = std::move(tracks);
            endInsertRows();
            mLoaded = true;
            emit loadedChanged();
        });
        watcher->setFuture(future);
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

    void PlaylistModel::removeTracks(std::vector<int> indexes)
    {
        ModelBatchRemover::removeIndexes(this, indexes);
        PlaylistUtils::instance()->savePlaylist(mFilePath, mTracks);
    }
}
