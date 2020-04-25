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

#include <functional>

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFutureWatcher>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
#include <QtConcurrentRun>

#include "libraryutils.h"
#include "modelutils.h"
#include "settings.h"

#include "abstractlibrarymodel.cpp"

namespace unplayer
{
    TracksModel::~TracksModel()
    {
        switch (mMode) {
        case AllTracksMode:
            Settings::instance()->setAllTracksSortSettings(mSortDescending, mSortMode, mInsideAlbumSortMode);
            break;
        case ArtistMode:
            Settings::instance()->setArtistTracksSortSettings(mSortDescending, mSortMode, mInsideAlbumSortMode);
            break;
        case AlbumAllArtistsMode:
        case AlbumSingleArtistMode:
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
        switch (mMode) {
        case AllTracksMode:
            mSortDescending = Settings::instance()->allTracksSortDescending();
            mSortMode = static_cast<SortMode>(Settings::instance()->allTracksSortMode(SortMode::ArtistAlbumYear));
            mInsideAlbumSortMode = static_cast<InsideAlbumSortMode>(Settings::instance()->allTracksInsideAlbumSortMode(InsideAlbumSortMode::DiscNumberTrackNumber));
            break;
        case ArtistMode:
            mSortDescending = Settings::instance()->artistTracksSortDescending();
            mSortMode = static_cast<SortMode>(Settings::instance()->artistTracksSortMode(SortMode::ArtistAlbumYear));
            mInsideAlbumSortMode = static_cast<InsideAlbumSortMode>(Settings::instance()->artistTracksInsideAlbumSortMode(InsideAlbumSortMode::DiscNumberTrackNumber));
            break;
        case AlbumAllArtistsMode:
        case AlbumSingleArtistMode:
            mSortDescending = Settings::instance()->albumTracksSortDescending();
            mSortMode = SortMode::ArtistAlbumTitle;
            mInsideAlbumSortMode = static_cast<InsideAlbumSortMode>(Settings::instance()->albumTracksSortMode(InsideAlbumSortMode::DiscNumberTrackNumber));
            break;
        default:
            break;
        }

        emit sortModeChanged();
        emit insideAlbumSortModeChanged();

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

    TracksModel::Mode TracksModel::mode() const
    {
        return mMode;
    }

    void TracksModel::setMode(TracksModel::Mode mode)
    {
        mMode = mode;
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

    QString TracksModel::makeQueryString(Mode mode,
                                         SortMode sortMode,
                                         InsideAlbumSortMode insideAlbumSortMode,
                                         bool sortDescending,
                                         int artistId,
                                         int albumId,
                                         int genreId,
                                         bool& groupTracks)
    {
        QString queryString(QLatin1String("SELECT tracks.id, filePath, tracks.title, duration, directoryMediaArt, embeddedMediaArt, %1 "
                                          "FROM tracks "
                                          "LEFT JOIN tracks_artists ON tracks_artists.trackId = tracks.id "
                                          "LEFT JOIN artists ON artists.id = tracks_artists.artistId "
                                          "LEFT JOIN tracks_albums ON tracks_albums.trackId = tracks.id "
                                          "LEFT JOIN albums ON albums.id = tracks_albums.albumId "));

        bool albumMode = false;
        groupTracks = true;

        switch (mode) {
        case AllTracksMode:
            break;
        case ArtistMode:
            if (artistId == 0) {
                queryString += QLatin1String("WHERE artists.id IS NULL ");
            } else {
                queryString += QString::fromLatin1("WHERE artists.id = %1 ").arg(artistId);
            }
            break;
        case AlbumAllArtistsMode:
            albumMode = true;
            if (albumId == 0) {
                queryString += QLatin1String("WHERE albums.id IS NULL ");
            } else {
                queryString += QString::fromLatin1("WHERE albums.id = %1 ").arg(albumId);
            }
            break;
        case AlbumSingleArtistMode:
            albumMode = true;
            groupTracks = false; // no need
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
        case GenreMode:
            queryString += QString::fromLatin1("JOIN tracks_genres ON tracks_genres.trackId = tracks.id "
                                               "JOIN genres ON genres.id = tracks_genres.genreId "
                                               "WHERE genres.id = %1 ").arg(genreId);
        }

        QString sortString;
        bool sortInsideAlbum = false;

        if (albumMode) {
            sortString = QLatin1String("ORDER BY ");
            sortInsideAlbum = true;
        } else {
            switch (sortMode) {
            case SortMode::Title:
                sortString = QLatin1String("ORDER BY tracks.title %1");
                break;
            case SortMode::AddedDate:
                sortString = QLatin1String("ORDER BY tracks.id %1");
                break;
            case SortMode::ArtistAlbumTitle:
            case SortMode::ArtistAlbumYear:
            {
                sortString = QLatin1String("ORDER BY artists.title IS NULL %1, artists.title %1, albums.title IS NULL %1, ");
                if (sortMode == SortMode::ArtistAlbumTitle) {
                    sortString += QLatin1String("albums.title %1, ");
                } else {
                    sortString += QLatin1String("year %1, albums.title %1, ");
                }
                groupTracks = false;
                sortInsideAlbum = true;
                break;
            }
            }
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

        if (sortInsideAlbum) {
            switch (insideAlbumSortMode) {
            case InsideAlbumSortMode::Title:
                sortString += QLatin1String("tracks.title %1");
                break;
            case InsideAlbumSortMode::DiscNumberTitle:
                sortString += QLatin1String("discNumber IS NULL %1, discNumber %1, tracks.title %1");
                break;
            case InsideAlbumSortMode::DiscNumberTrackNumber:
                sortString += QLatin1String("discNumber IS NULL %1, discNumber %1, trackNumber %1, tracks.title %1");
                break;
            }
        }

        queryString += sortString.arg(sortDescending ? QLatin1String("DESC") : QLatin1String("ASC"));

        return queryString;
    }

    LibraryTrack TracksModel::trackFromQuery(const QSqlQuery& query, bool groupTracks)
    {
        const auto rejoin = [&](int field) {
            QString value(query.value(field).toString());
            if (groupTracks && value.contains(QLatin1Char('\n'))) {
                QStringList list(value.split(QLatin1Char('\n'), QString::SkipEmptyParts));
                if (list.size() > 1) {
                    list.removeDuplicates();
                    value = list.join(QLatin1String(", "));
                }
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
                            query.value(DurationField).toInt(),
                            /*mediaArtFromQuery(query, DirectoryMediaArtField, EmbeddedMediaArtField)*/QString()};
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
        return makeQueryString(mMode,
                               mSortMode,
                               mInsideAlbumSortMode,
                               mSortDescending,
                               mArtistId,
                               mAlbumId,
                               mGenreId,
                               mGroupTracks);
    }

    LibraryTrack TracksModel::itemFromQuery(const QSqlQuery& query)
    {
        return trackFromQuery(query, mGroupTracks);
    }
}
