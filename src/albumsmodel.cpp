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
#include "utils.h"

namespace unplayer
{
    namespace
    {
        enum Field
        {
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
        default:
            return QVariant();
        }
    }

    int AlbumsModel::rowCount(const QModelIndex&) const
    {
        return static_cast<int>(mAlbums.size());
    }

    bool AlbumsModel::removeRows(int row, int count, const QModelIndex& parent)
    {
        beginRemoveRows(parent, row, row + count - 1);
        const auto first(mAlbums.begin() + row);
        mAlbums.erase(first, first + count);
        endRemoveRows();
        return true;
    }

    bool AlbumsModel::allArtists() const
    {
        return mAllArtists;
    }

    void AlbumsModel::setAllArtists(bool allArtists)
    {
        mAllArtists = allArtists;
        emit allArtistsChanged();
    }

    const QString& AlbumsModel::artist() const
    {
        return mArtist;
    }

    void AlbumsModel::setArtist(const QString& artist)
    {
        mArtist = artist;
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
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT filePath, title, artist, album, duration, directoryMediaArt, embeddedMediaArt FROM tracks "
                                     "WHERE artist = ? AND album = ? "
                                     "ORDER BY trackNumber, title"));
        const Album& album = mAlbums[static_cast<size_t>(index)];
        query.addBindValue(album.artist);
        query.addBindValue(album.album);
        if (query.exec()) {
            std::vector<LibraryTrack> tracks;
            query.last();
            if (query.at() >= 0) {
                tracks.reserve(static_cast<size_t>(query.at() + 1));
                query.seek(QSql::BeforeFirstRow);
            }
            while (query.next()) {
                tracks.push_back({query.value(0).toString(),
                                  query.value(1).toString(),
                                  query.value(2).toString(),
                                  query.value(3).toString(),
                                  query.value(4).toInt(),
                                  mediaArtFromQuery(query, 5, 6)});
            }
            return tracks;
        }
        qWarning() << "failed to get tracks from database" << query.lastError();
        return {};
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
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT filePath FROM tracks "
                                     "WHERE artist = ? AND album = ? "
                                     "ORDER BY trackNumber, title"));
        const Album& album = mAlbums[static_cast<size_t>(index)];
        query.addBindValue(album.artist);
        query.addBindValue(album.album);
        if (query.exec()) {
            QStringList tracks;
            query.last();
            if (query.at() >= 0) {
                tracks.reserve(query.at() + 1);
                query.seek(QSql::BeforeFirstRow);
            }
            while (query.next()) {
                tracks.push_back(query.value(0).toString());
            }
            return tracks;
        }
        qWarning() << "failed to get tracks from database" << query.lastError();
        return {};
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

        std::vector<Album> albums;
        albums.reserve(indexes.size());
        for (int index : indexes) {
            albums.push_back(mAlbums[static_cast<size_t>(index)]);
        }
        QObject::connect(LibraryUtils::instance(), &LibraryUtils::removingFilesChanged, this, [this, indexes] {
            if (!LibraryUtils::instance()->isRemovingFiles()) {
                ModelBatchRemover remover(this);
                for (int i = static_cast<int>(indexes.size()) - 1; i >= 0; --i) {
                    remover.remove(indexes[static_cast<size_t>(i)]);
                }
                remover.remove();
                QObject::disconnect(LibraryUtils::instance(), &LibraryUtils::removingFilesChanged, this, nullptr);
            }
        });
        LibraryUtils::instance()->removeAlbums(std::move(albums), deleteFiles);
    }

    QHash<int, QByteArray> AlbumsModel::roleNames() const
    {
        return {{ArtistRole, "artist"},
                {DisplayedArtistRole, "displayedArtist"},
                {UnknownArtistRole, "unknownArtist"},
                {AlbumRole, "album"},
                {DisplayedAlbumRole, "displayedAlbum"},
                {UnknownAlbumRole, "unknownAlbum"},
                {YearRole, "year"},
                {TracksCountRole, "tracksCount"},
            {DurationRole, "duration"}};
    }

    void AlbumsModel::execQuery()
    {
        QString queryString(QLatin1String("SELECT %1, album, year, COUNT(*), SUM(duration) FROM "
                                          "(SELECT %1, album, year, duration FROM tracks GROUP BY id, %1, album) "));
        if (mAllArtists) {
            queryString += QLatin1String("GROUP BY album, %1 ");
        } else {
            queryString += QLatin1String("WHERE %1 = ? GROUP BY album ");
        }

        switch (mSortMode) {
        case SortAlbum:
            queryString += QLatin1String("ORDER BY album = '' %2, album %2");
            break;
        case SortYear:
            queryString += QLatin1String("ORDER BY year %2, album = '' %2, album %2");
            break;
        case SortArtistAlbum:
            queryString += QLatin1String("ORDER BY %1 = '' %2, %1 %2, album = '' %2, album %2");
            break;
        case SortArtistYear:
            queryString += QLatin1String("ORDER BY %1 = '' %2, %1 %2, year %2, album = '' %2, album %2");
        }

        queryString = queryString.arg(Settings::instance()->useAlbumArtist() ? QLatin1String("albumArtist") : QLatin1String("artist"),
                                      mSortDescending ? QLatin1String("DESC") : QLatin1String("ASC"));


        QSqlQuery query;
        query.prepare(queryString);
        if (!mAllArtists) {
            query.addBindValue(mArtist);
        }

        beginResetModel();
        mAlbums.clear();
        if (query.exec()) {
            while (query.next()) {
                const QString artist(query.value(ArtistField).toString());
                const QString album(query.value(AlbumField).toString());
                mAlbums.push_back({artist,
                                   artist.isEmpty() ? qApp->translate("unplayer", "Unknown artist") : artist,
                                   album,
                                   album.isEmpty() ? qApp->translate("unplayer", "Unknown album") : album,
                                   query.value(YearField).toInt(),
                                   query.value(TracksCountField).toInt(),
                                   query.value(DurationField).toInt()});
            }
        } else {
            qWarning() << query.lastError();
        }
        endResetModel();
    }
}
