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

#include "tracksmodel.h"

#include <QCoreApplication>
#include <QSqlQuery>

#include "libraryutils.h"
#include "modelutils.h"
#include "settings.h"

#include "abstractlibrarymodel.cpp"

namespace unplayer
{
    TracksModel::~TracksModel()
    {
        switch (mQueryMode) {
        case QueryAllTracks:
            Settings::instance()->setAllTracksSortSettings(mSortDescending, mSortMode, mInsideAlbumSortMode);
            break;
        case QueryArtistTracks:
            Settings::instance()->setArtistTracksSortSettings(mSortDescending, mSortMode, mInsideAlbumSortMode);
            break;
        case QueryAlbumTracksForAllArtists:
        case QueryAlbumTracksForSingleArtist:
            Settings::instance()->setAlbumTracksSortSettings(mSortDescending, mInsideAlbumSortMode);
            break;
        default:
            break;
        }
    }

    void TracksModel::classBegin()
    {

    }

    void TracksModel::componentComplete()
    {
        switch (mQueryMode) {
        case QueryAllTracks:
            setSortDescending(Settings::instance()->allTracksSortDescending());
            setSortMode(TracksModelSortMode::fromSettingsForAllTracks());
            setInsideAlbumSortMode(TracksModelInsideAlbumSortMode::fromSettingsForAllTracks());
            break;
        case QueryArtistTracks:
            setSortDescending(Settings::instance()->artistTracksSortDescending());
            setSortMode(TracksModelSortMode::fromSettingsForArtist());
            setInsideAlbumSortMode(TracksModelInsideAlbumSortMode::fromSettingsForArtist());
            break;
        case QueryAlbumTracksForAllArtists:
        case QueryAlbumTracksForSingleArtist:
            setSortDescending(Settings::instance()->albumTracksSortDescending());
            setSortMode({}); // ignored
            setInsideAlbumSortMode(TracksModelInsideAlbumSortMode::fromSettingsForAlbum());
            break;
        case QueryGenreTracks:
            setSortDescending(false);
            setSortMode(SortMode::Artist_AlbumYear);
            setInsideAlbumSortMode(InsideAlbumSortMode::DiscNumber_TrackNumber);
            break;
        }

        execQuery();
    }

    QVariant TracksModel::data(const QModelIndex& index, int role) const
    {
        const LibraryTrack& track = mTracks[static_cast<size_t>(index.row())];

        switch (role) {
        case FilePathRole:
            return track.filePath;
        case TitleRole:
            return track.title;
        case ArtistRole:
            return track.artist;
        case AlbumRole:
            return track.album;
        case DurationRole:
            return track.duration;
        default:
            return QVariant();
        }
    }

    TracksModel::QueryMode TracksModel::queryMode() const
    {
        return mQueryMode;
    }

    void TracksModel::setQueryMode(TracksModel::QueryMode mode)
    {
        if (mode != mQueryMode) {
            mQueryMode = mode;
            emit queryModeChanged(mode);
        }
    }

    int TracksModel::artistId() const
    {
        return mArtistId;
    }

    void TracksModel::setArtistId(int id)
    {
        mArtistId = id;
    }

    int TracksModel::albumId() const
    {
        return mAlbumId;
    }

    void TracksModel::setAlbumId(int id)
    {
        mAlbumId = id;
    }

    int TracksModel::genreId() const
    {
        return mGenreId;
    }

    void TracksModel::setGenreId(int id)
    {
        mGenreId = id;
    }

    bool TracksModel::sortDescending() const
    {
        return mSortDescending;
    }

    void TracksModel::setSortDescending(bool descending)
    {
        if (descending != mSortDescending) {
            mSortDescending = descending;
            execQuery();
        }
    }

    TracksModel::SortMode TracksModel::sortMode() const
    {
        return mSortMode;
    }

    void TracksModel::setSortMode(TracksModel::SortMode mode)
    {
        if (mode != mSortMode) {
            mSortMode = mode;
            emit sortModeChanged();
            execQuery();
        }
    }

    TracksModel::InsideAlbumSortMode TracksModel::insideAlbumSortMode() const
    {
        return mInsideAlbumSortMode;
    }

    void TracksModel::setInsideAlbumSortMode(TracksModel::InsideAlbumSortMode mode)
    {
        if (mode != mInsideAlbumSortMode) {
            mInsideAlbumSortMode = mode;
            emit insideAlbumSortModeChanged();
            execQuery();
        }
    }

    std::vector<LibraryTrack> TracksModel::getTracks(const std::vector<int>& indexes) const
    {
        std::vector<LibraryTrack> tracks;
        tracks.reserve(indexes.size());
        for (int index : indexes) {
            tracks.push_back(mTracks[static_cast<size_t>(index)]);
        }
        return tracks;
    }

    LibraryTrack TracksModel::getTrack(int index) const
    {
        return mTracks[static_cast<size_t>(index)];
    }

    QStringList TracksModel::getTrackPaths(const std::vector<int>& indexes) const
    {
        QStringList tracks;
        tracks.reserve(static_cast<int>(indexes.size()));
        for (int index : indexes) {
            tracks.push_back(mTracks[static_cast<size_t>(index)].filePath);
        }
        return tracks;
    }

    void TracksModel::removeTrack(int index, bool deleteFile)
    {
        removeTracks({index}, deleteFile);
    }

    void TracksModel::removeTracks(const std::vector<int>& indexes, bool deleteFiles)
    {
        if (LibraryUtils::instance()->isRemovingFiles()) {
            return;
        }

        std::vector<LibraryTrack> files;
        files.reserve(indexes.size());
        for (int index : indexes) {
            files.push_back(mTracks[static_cast<size_t>(index)]);
        }
        LibraryUtils::instance()->removeTracks(std::move(files), deleteFiles);
        QObject::connect(LibraryUtils::instance(), &LibraryUtils::removingFilesChanged, this, [this, indexes] {
            if (!LibraryUtils::instance()->isRemovingFiles()) {
                ModelBatchRemover::removeIndexes(this, indexes);
                QObject::disconnect(LibraryUtils::instance(), &LibraryUtils::removingFilesChanged, this, nullptr);
            }
        });
    }

        namespace
    {
    namespace query
    {
        QString select()
        {
            return QLatin1String("SELECT tracks.id, filePath, tracks.title, duration, %1 "
                                 "FROM tracks ");
        }

        void join(QString& queryString, TracksModel::QueryMode queryMode)
        {
            queryString += QLatin1String("LEFT JOIN tracks_artists ON tracks_artists.trackId = tracks.id "
                                         "LEFT JOIN artists ON artists.id = tracks_artists.artistId "
                                         "LEFT JOIN tracks_albums ON tracks_albums.trackId = tracks.id "
                                         "LEFT JOIN albums ON albums.id = tracks_albums.albumId ");
            if (queryMode == TracksModel::QueryGenreTracks) {
                queryString += QLatin1String("JOIN tracks_genres ON tracks_genres.trackId = tracks.id "
                                             "JOIN genres ON genres.id = tracks_genres.genreId ");
            }
        }

        void where(QString& queryString,
                   TracksModel::QueryMode queryMode,
                   int artistId,
                   int albumId,
                   int genreId)
        {
            switch (queryMode) {
            case TracksModel::QueryAllTracks:
                break;
            case TracksModel::QueryArtistTracks:
                if (artistId == 0) {
                    queryString += QLatin1String("WHERE artists.id IS NULL ");
                } else {
                    queryString += QString::fromLatin1("WHERE artists.id = %1 ").arg(artistId);
                }
                break;
            case TracksModel::QueryAlbumTracksForAllArtists:
                if (albumId == 0) {
                    queryString += QLatin1String("WHERE albums.id IS NULL ");
                } else {
                    queryString += QString::fromLatin1("WHERE albums.id = %1 ").arg(albumId);
                }
                break;
            case TracksModel::QueryAlbumTracksForSingleArtist:
                if (artistId == 0) {
                    queryString += QLatin1String("WHERE artists.id IS NULL ");
                } else {
                    queryString += QString::fromLatin1("WHERE artists.id = %1 ").arg(artistId);
                }
                if (albumId == 0) {
                    queryString += QLatin1String("AND albums.id IS NULL ");
                } else {
                    queryString += QString::fromLatin1("AND albums.id = %1 ").arg(albumId);
                }
                break;
            case TracksModel::QueryGenreTracks:
                queryString += QString::fromLatin1("WHERE genres.id = %1 ").arg(genreId);
                break;
            }
        }

        void group(QString& queryString,
                   TracksModel::QueryMode queryMode,
                   TracksModel::SortMode sortMode,
                   bool& groupTracks) {
            groupTracks = true;
            switch (queryMode) {
            case TracksModel::QueryAlbumTracksForAllArtists:
                break;
            case TracksModel::QueryAlbumTracksForSingleArtist:
                groupTracks = false;
                break;
            default:
                switch (sortMode) {
                case TracksModel::SortMode::Artist_AlbumTitle:
                case TracksModel::SortMode::Artist_AlbumYear:
                    groupTracks = false;
                    break;
                default:
                    break;
                }
                break;
            }
            if (groupTracks) {
                // For some reason this hack (concatenating by newline and then splitting, removing duplicates and concatenating by ', ')
                // is faster than group_concat(DISTINCT ' ' || artists.title) that produces the same output (after you remove leading space)
                // It will probably be slower for tracks with large number or artists/albums, but this is unlikely
                queryString = queryString.arg(QLatin1String("group_concat(artists.title, '\n'), group_concat(albums.title, '\n')"));
                queryString += QLatin1String("GROUP BY tracks.id ");
            } else {
                queryString = queryString.arg(QLatin1String("artists.title, albums.title"));
            }
        }

        void sort(QString& queryString,
                  TracksModel::QueryMode queryMode,
                  TracksModel::SortMode sortMode,
                  TracksModel::InsideAlbumSortMode insideAlbumSortMode,
                  bool sortDescending)
        {
            QString sortString;

            bool sortInsideAlbum = false;

            switch (queryMode) {
            case TracksModel::QueryAlbumTracksForAllArtists:
            case TracksModel::QueryAlbumTracksForSingleArtist:
                sortString = QLatin1String("ORDER BY ");
                sortInsideAlbum = true;
                break;
            default:
                switch (sortMode) {
                case TracksModel::SortMode::Title:
                    sortString = QLatin1String("ORDER BY tracks.title %1");
                    break;
                case TracksModel::SortMode::AddedDate:
                    sortString = QLatin1String("ORDER BY tracks.id %1");
                    break;
                case TracksModel::SortMode::Artist_AlbumTitle:
                case TracksModel::SortMode::Artist_AlbumYear:
                {
                    sortString = QLatin1String("ORDER BY artists.title IS NULL %1, artists.title %1, albums.title IS NULL %1, ");
                    if (sortMode == TracksModel::SortMode::Artist_AlbumTitle) {
                        sortString += QLatin1String("albums.title %1, ");
                    } else {
                        sortString += QLatin1String("year %1, albums.title %1, ");
                    }
                    sortInsideAlbum = true;
                    break;
                }
                }
                break;
            }

            if (sortInsideAlbum) {
                switch (insideAlbumSortMode) {
                case TracksModel::InsideAlbumSortMode::Title:
                    sortString += QLatin1String("tracks.title %1");
                    break;
                case TracksModel::InsideAlbumSortMode::DiscNumber_Title:
                    sortString += QLatin1String("discNumber IS NULL %1, discNumber %1, tracks.title %1");
                    break;
                case TracksModel::InsideAlbumSortMode::DiscNumber_TrackNumber:
                    sortString += QLatin1String("discNumber IS NULL %1, discNumber %1, trackNumber %1, tracks.title %1");
                    break;
                }
            }

            queryString += sortString.arg(sortDescending ? QLatin1String("DESC") : QLatin1String("ASC"));
        }
        }
    }

    QString TracksModel::makeQueryString(QueryMode queryMode,
                                         SortMode sortMode,
                                         InsideAlbumSortMode insideAlbumSortMode,
                                         bool sortDescending,
                                         int artistId,
                                         int albumId,
                                         int genreId,
                                         bool& groupTracks)
    {
        qInfo().nospace() << "queryMode = " << queryMode
                        << ", sortMode = " << sortMode
                        << ", insideAlbumSortMode = " << insideAlbumSortMode
                        << ", sortDescending = " << sortDescending
                        << ", artistId = " << artistId
                        << ", albumId = " << albumId
                        << ", genreId = " << genreId;
        QString queryString(query::select());
        query::join(queryString, queryMode);
        query::where(queryString, queryMode, artistId, albumId, genreId);
        query::group(queryString, queryMode, sortMode, groupTracks);
        query::sort(queryString, queryMode, sortMode, insideAlbumSortMode, sortDescending);
        return queryString;
    }

    LibraryTrack TracksModel::trackFromQuery(const QSqlQuery& query, bool groupTracks)
    {
        bool filteredSingleAlbum = false;
        const auto rejoin = [&](int field) {
            QString value(query.value(field).toString());
            if (groupTracks) {
                if (value.contains(QLatin1Char('\n'))) {
                    QStringList list(value.split(QLatin1Char('\n'), QString::SkipEmptyParts));
                    if (list.size() > 1) {
                        list.removeDuplicates();
                        value = list.join(QLatin1String(", "));
                    }
                }
            } else if (!value.isEmpty() && field == AlbumField) {
                filteredSingleAlbum = true;
            }
            return value;
        };
        const QString artist(rejoin(ArtistField));
        const QString album(rejoin(AlbumField));
        LibraryTrack track {query.value(IdField).toInt(),
                            query.value(FilePathField).toString(),
                            query.value(TitleField).toString(),
                            artist.isEmpty() ? qApp->translate("unplayer", "Unknown artist") : artist,
                            album.isEmpty() ? qApp->translate("unplayer", "Unknown album") : album,
                            filteredSingleAlbum,
                            query.value(DurationField).toInt()};
        return track;
    }

    QHash<int, QByteArray> TracksModel::roleNames() const
    {
        return {{FilePathRole, "filePath"},
                {TitleRole, "title"},
                {ArtistRole, "artist"},
                {AlbumRole, "album"},
                {DurationRole, "duration"}};
    }

    QString TracksModel::makeQueryString()
    {
        return makeQueryString(mQueryMode,
                               mSortMode,
                               mInsideAlbumSortMode,
                               mSortDescending,
                               mArtistId,
                               mAlbumId,
                               mGenreId,
                               mGroupTracks);
    }

    TracksModel::AbstractItemFactory* TracksModel::createItemFactory()
    {
        return new ItemFactory(mGroupTracks);
    }

    LibraryTrack TracksModel::ItemFactory::itemFromQuery(const QSqlQuery& query)
    {
        return TracksModel::trackFromQuery(query, groupTracks);
    }

    TracksModelSortMode::Mode TracksModelSortMode::fromInt(int value)
    {
        switch (value) {
        case Title:
        case AddedDate:
        case Artist_AlbumTitle:
        case Artist_AlbumYear:
            return static_cast<Mode>(value);
        }
        qWarning("Failed to convert value %d to SortMode", value);
        return {};
    }

    TracksModelSortMode::Mode TracksModelSortMode::fromSettingsForAllTracks()
    {
        return fromInt(Settings::instance()->allTracksSortMode(Title));
    }

    TracksModelSortMode::Mode TracksModelSortMode::fromSettingsForArtist()
    {
        return fromInt(Settings::instance()->artistTracksSortMode(Title));
    }

    TracksModelInsideAlbumSortMode::Mode TracksModelInsideAlbumSortMode::fromInt(int value)
    {
        switch (value) {
        case Title:
        case DiscNumber_Title:
        case DiscNumber_TrackNumber:
            return static_cast<Mode>(value);
        }
        qWarning("Failed to convert value %d to InsideAlbumSortMode", value);
        return {};
    }

    TracksModelInsideAlbumSortMode::Mode TracksModelInsideAlbumSortMode::fromSettingsForAllTracks()
    {
        return fromInt(Settings::instance()->allTracksInsideAlbumSortMode(DiscNumber_TrackNumber));
    }

    TracksModelInsideAlbumSortMode::Mode TracksModelInsideAlbumSortMode::fromSettingsForArtist()
    {
        return fromInt(Settings::instance()->artistTracksInsideAlbumSortMode(DiscNumber_TrackNumber));
    }

    TracksModelInsideAlbumSortMode::Mode TracksModelInsideAlbumSortMode::fromSettingsForAlbum()
    {
        return fromInt(Settings::instance()->albumTracksSortMode(DiscNumber_TrackNumber));
    }

}
