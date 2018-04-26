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

#include "albumsmodel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QSqlError>

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

        setQuery();
    }

    QVariant AlbumsModel::data(const QModelIndex& index, int role) const
    {
        mQuery->seek(index.row());

        switch (role) {
        case ArtistRole:
            return mQuery->value(ArtistField);
        case DisplayedArtistRole:
        {
            const QString artist(mQuery->value(ArtistField).toString());
            if (artist.isEmpty()) {
                return qApp->translate("unplayer", "Unknown artist");
            }
            return artist;
        }
        case UnknownArtistRole:
            return mQuery->value(ArtistField).toString().isEmpty();
        case AlbumRole:
            return mQuery->value(AlbumField);
        case DisplayedAlbumRole:
        {
            const QString album(mQuery->value(AlbumField).toString());
            if (album.isEmpty()) {
                return qApp->translate("unplayer", "Unknown album");
            }
            return album;
        }
        case UnknownAlbumRole:
            return mQuery->value(AlbumField).toString().isEmpty();
        case YearRole:
            return mQuery->value(YearField).toInt();
        case TracksCountRole:
            return mQuery->value(TracksCountField);
        case DurationRole:
            return mQuery->value(DurationField);
        default:
            return QVariant();
        }
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
            setQuery();
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
            setQuery();
        }
    }

    QStringList AlbumsModel::getTracksForAlbum(int index) const
    {
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT filePath FROM tracks "
                                     "WHERE artist = ? AND album = ? "
                                     "ORDER BY trackNumber, title"));
        mQuery->seek(index);
        query.addBindValue(mQuery->value(ArtistField).toString());
        query.addBindValue(mQuery->value(AlbumField).toString());
        if (query.exec()) {
            QStringList tracks;
            while (query.next()) {
                tracks.append(query.value(0).toString());
            }
            return tracks;
        }

        qWarning() << "failed to get tracks from database" << query.lastError();
        return QStringList();
    }

    QStringList AlbumsModel::getTracksForAlbums(const QVector<int>& indexes) const
    {
        QStringList tracks;
        QSqlDatabase::database().transaction();
        for (int index : indexes) {
            tracks.append(getTracksForAlbum(index));
        }
        QSqlDatabase::database().commit();
        return tracks;
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

    void AlbumsModel::setQuery()
    {
        QString query(QLatin1String("SELECT artist, album, year, COUNT(*), SUM(duration) FROM "
                                    "(SELECT artist, album, year, duration FROM tracks GROUP BY id, artist, album) "));
        if (mAllArtists) {
            query += QLatin1String("GROUP BY album, artist ");
        } else {
            query += QLatin1String("WHERE artist = ? "
                                   "GROUP BY album ");
        }

        switch (mSortMode) {
        case SortAlbum:
            query += QLatin1String("ORDER BY album = '' %1, album %1");
            break;
        case SortYear:
            query += QLatin1String("ORDER BY year %1, album = '' %1, album %1");
            break;
        case SortArtistAlbum:
            query += QLatin1String("ORDER BY artist = '' %1, artist %1, album = '' %1, album %1");
            break;
        case SortArtistYear:
            query += QLatin1String("ORDER BY artist = '' %1, artist %1, year %1, album = '' %1, album %1");
        }

        query = query.arg(mSortDescending ? QLatin1String("DESC")
                                          : QLatin1String("ASC"));


        beginResetModel();
        mQuery->prepare(query);
        if (!mAllArtists) {
            mQuery->addBindValue(mArtist);
        }
        execQuery();
        endResetModel();
    }
}
