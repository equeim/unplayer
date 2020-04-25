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

#include "albumsmodel.h"

#include <functional>

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFutureWatcher>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtConcurrentRun>

#include "libraryutils.h"
#include "modelutils.h"
#include "settings.h"
#include "tracksmodel.h"
#include "utils.h"

#include "abstractlibrarymodel.cpp"

namespace unplayer
{
    namespace
    {
        enum Field
        {
            IdField,
            ArtistField,
            AlbumField,
            YearField,
            TracksCountField,
            DurationField
        };
    }

    AlbumsModel::~AlbumsModel()
    {
        if (mAllArtists) {
            Settings::instance()->setAllAlbumsSortSettings(mSortDescending, mSortMode);
        } else {
            Settings::instance()->setAlbumsSortSettings(mSortDescending, mSortMode);
        }
    }

    void AlbumsModel::classBegin()
    {

    }

    void AlbumsModel::componentComplete()
    {
        if (mAllArtists) {
            mSortDescending = Settings::instance()->allAlbumsSortDescending();
            mSortMode = static_cast<SortMode>(Settings::instance()->allAlbumsSortMode(SortArtistYear));
        } else {
            mSortDescending = Settings::instance()->albumsSortDescending();
            mSortMode = static_cast<SortMode>(Settings::instance()->albumsSortMode(SortYear));
        }

        emit sortModeChanged();

        execQuery();
    }

    QVariant AlbumsModel::data(const QModelIndex& index, int role) const
    {
        const Album& album = mAlbums[static_cast<size_t>(index.row())];

        switch (role) {
        case AlbumIdRole:
            return album.id;
        case ArtistRole:
            return album.artist;
        case DisplayedArtistRole:
            return album.displayedArtist;
        case UnknownArtistRole:
            return album.artist.isEmpty();
        case AlbumRole:
            return album.album;
        case DisplayedAlbumRole:
            return album.displayedAlbum;
        case UnknownAlbumRole:
            return album.album.isEmpty();
        case YearRole:
            return album.year;
        case TracksCountRole:
            return album.tracksCount;
        case DurationRole:
            return album.duration;
        }

        return QVariant();
    }

    bool AlbumsModel::allArtists() const
    {
        return mAllArtists;
    }

    void AlbumsModel::setAllArtists(bool allArtists)
    {
        if (allArtists != mAllArtists) {
            mAllArtists = allArtists;
            emit allArtistsChanged();
        }
    }

    int AlbumsModel::artistId() const
    {
        return mArtistId;
    }

    void AlbumsModel::setArtistId(int id)
    {
        mArtistId = id;
    }

    bool AlbumsModel::sortDescending() const
    {
        return mSortDescending;
    }

    void AlbumsModel::setSortDescending(bool descending)
    {
        if (descending != mSortDescending) {
            mSortDescending = descending;
            execQuery();
        }
    }

    AlbumsModel::SortMode AlbumsModel::sortMode() const
    {
        return mSortMode;
    }

    void AlbumsModel::setSortMode(AlbumsModel::SortMode mode)
    {
        if (mode != mSortMode) {
            mSortMode = mode;
            emit sortModeChanged();
            execQuery();
        }
    }

    std::vector<LibraryTrack> AlbumsModel::getTracksForAlbum(int index) const
    {
        std::vector<LibraryTrack> tracks;

        const Album& album = mAlbums[static_cast<size_t>(index)];
        bool groupTracks = false;
        QSqlQuery query;
        if (query.exec(TracksModel::makeQueryString(mAllArtists ? TracksModel::AlbumAllArtistsMode : TracksModel::AlbumSingleArtistMode,
                                                    TracksModel::SortMode::ArtistAlbumYear,
                                                    TracksModel::InsideAlbumSortMode::DiscNumberTrackNumber,
                                                    false,
                                                    mArtistId,
                                                    album.id,
                                                    0,
                                                    groupTracks))) {
            if (query.last()) {
                tracks.reserve(static_cast<size_t>(query.at() + 1));
                query.seek(QSql::BeforeFirstRow);
                while (query.next()) {
                    tracks.push_back(TracksModel::trackFromQuery(query, groupTracks));
                }
            }
        } else {
            qWarning() << __func__ << query.lastError();
        }

        return tracks;
    }

    std::vector<LibraryTrack> AlbumsModel::getTracksForAlbums(const std::vector<int>& indexes) const
    {
        std::vector<LibraryTrack> tracks;
        QSqlDatabase::database().transaction();
        for (int index : indexes) {
            std::vector<LibraryTrack> albumTracks(getTracksForAlbum(index));
            tracks.insert(tracks.end(), std::make_move_iterator(albumTracks.begin()), std::make_move_iterator(albumTracks.end()));
        }
        QSqlDatabase::database().commit();
        return tracks;
    }

    QStringList AlbumsModel::getTrackPathsForAlbum(int index) const
    {
        QStringList tracks;

        const Album& album = mAlbums[static_cast<size_t>(index)];
        bool groupTracks = false;
        QSqlQuery query;
        if (query.exec(TracksModel::makeQueryString(mAllArtists ? TracksModel::AlbumAllArtistsMode : TracksModel::AlbumSingleArtistMode,
                                                    TracksModel::SortMode::ArtistAlbumYear,
                                                    TracksModel::InsideAlbumSortMode::DiscNumberTrackNumber,
                                                    false,
                                                    mArtistId,
                                                    album.id,
                                                    0,
                                                    groupTracks))) {
            if (query.last()) {
                tracks.reserve(query.at() + 1);
                query.seek(QSql::BeforeFirstRow);
                while (query.next()) {
                    tracks.push_back(query.value(TracksModel::FilePathField).toString());
                }
            }
        } else {
            qWarning() << __func__ << query.lastError();
        }

        return tracks;
    }

    QStringList AlbumsModel::getTrackPathsForAlbums(const std::vector<int>& indexes) const
    {
        QStringList tracks;

        QSqlDatabase::database().transaction();
        for (int index : indexes) {
            QStringList albumTracks(getTrackPathsForAlbum(index));
            tracks.append(getTrackPathsForAlbum(index));
        }
        QSqlDatabase::database().commit();
        return tracks;
    }

    void AlbumsModel::removeAlbum(int index, bool deleteFiles)
    {
        removeAlbums({index}, deleteFiles);
    }

    void AlbumsModel::removeAlbums(const std::vector<int>& indexes, bool deleteFiles)
    {
        if (LibraryUtils::instance()->isRemovingFiles()) {
            return;
        }

        std::vector<int> albums;
        albums.reserve(indexes.size());
        for (int index : indexes) {
            albums.push_back(mAlbums[static_cast<size_t>(index)].id);
        }
        LibraryUtils::instance()->removeAlbums(std::move(albums), deleteFiles);
        QObject::connect(LibraryUtils::instance(), &LibraryUtils::removingFilesChanged, this, [this, indexes] {
            if (!LibraryUtils::instance()->isRemovingFiles()) {
                ModelBatchRemover::removeIndexes(this, indexes);
                QObject::disconnect(LibraryUtils::instance(), &LibraryUtils::removingFilesChanged, this, nullptr);
            }
        });
    }

    QHash<int, QByteArray> AlbumsModel::roleNames() const
    {
        return {{AlbumIdRole, "albumId"},
                {ArtistRole, "artist"},
                {DisplayedArtistRole, "displayedArtist"},
                {UnknownArtistRole, "unknownArtist"},
                {AlbumRole, "album"},
                {DisplayedAlbumRole, "displayedAlbum"},
                {UnknownAlbumRole, "unknownAlbum"},
                {YearRole, "year"},
                {TracksCountRole, "tracksCount"},
                {DurationRole, "duration"}};
    }

    QString AlbumsModel::makeQueryString()
    {
        QString queryString;
        if (mAllArtists) {
            queryString = QLatin1String("SELECT albumId, group_concat(artistTitle0, ', ') as artistTitle, albumTitle, year, tracksCount, duration "
                                        "FROM ("
                                            "SELECT albums.id AS albumId, artists.title AS artistTitle0, albums.title AS albumTitle, year, COUNT(tracks.id) AS tracksCount, SUM(duration) AS duration "
                                            "FROM tracks "
                                            "LEFT JOIN tracks_albums ON tracks_albums.trackId = tracks.id "
                                            "LEFT JOIN albums ON albums.id = tracks_albums.albumId "
                                            "LEFT JOIN albums_artists ON albums_artists.albumId = albums.id "
                                            "LEFT JOIN artists ON artists.id = albums_artists.artistId "
                                            "GROUP BY artists.id, albums.id"
                                        ") "
                                        "GROUP BY albumId ");
        } else {
            queryString = QLatin1String("SELECT albums.id AS albumId, artists.title AS artistTitle, albums.title AS albumTitle, year, COUNT(tracks.id) AS tracksCount, SUM(duration) AS duration "
                                        "FROM tracks "
                                        "LEFT JOIN tracks_artists ON tracks_artists.trackId = tracks.id "
                                        "LEFT JOIN artists ON artists.id = tracks_artists.artistId "
                                        "LEFT JOIN tracks_albums ON tracks_albums.trackId = tracks.id "
                                        "LEFT JOIN albums ON albums.id = tracks_albums.albumId ");
            if (mArtistId == 0) {
                queryString += QLatin1String("WHERE artists.id IS NULL ");
            } else {
                queryString += QString::fromLatin1("WHERE artists.id = %1 ").arg(mArtistId);
            }

            queryString += QLatin1String("GROUP BY albums.id ");
        }

        switch (mSortMode) {
        case SortAlbum:
            queryString += QLatin1String("ORDER BY albumTitle IS NULL %1, albumTitle %1");
            break;
        case SortYear:
            queryString += QLatin1String("ORDER BY year %1, albumTitle IS NULL %1, albumTitle %1");
            break;
        case SortArtistAlbum:
            queryString += QLatin1String("ORDER BY artistTitle IS NULL %1, artistTitle %1, albumTitle IS NULL %1, albumTitle %1");
            break;
        case SortArtistYear:
            queryString += QLatin1String("ORDER BY artistTitle IS NULL %1, artistTitle %1, year %1, albumTitle IS NULL %1, albumTitle %1");
        }

        return queryString.arg(mSortDescending ? QLatin1String("DESC") : QLatin1String("ASC"));
    }

    Album AlbumsModel::itemFromQuery(const QSqlQuery& query)
    {
        const QString artist(query.value(ArtistField).toString());
        const QString album(query.value(AlbumField).toString());
        return {query.value(IdField).toInt(),
                artist,
                artist.isEmpty() ? qApp->translate("unplayer", "Unknown artist") : artist,
                album,
                album.isEmpty() ? qApp->translate("unplayer", "Unknown album") : album,
                query.value(YearField).toInt(),
                query.value(TracksCountField).toInt(),
                query.value(DurationField).toInt()};
    }
}
