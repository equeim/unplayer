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

#include "albumsmodel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QSqlError>

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

    AlbumsModel::AlbumsModel()
        : mAllArtists(true)
    {

    }

    void AlbumsModel::classBegin()
    {

    }

    void AlbumsModel::componentComplete()
    {
        QString query(QLatin1String("SELECT artist, album, year, COUNT(*), SUM(duration) FROM "
                                    "(SELECT artist, album, year, duration FROM tracks GROUP BY filePath, artist, album) "));
        if (mAllArtists) {
            query += QLatin1String("GROUP BY album, artist ");
        } else {
            query += QLatin1String("WHERE artist = ? "
                                   "GROUP BY album ");
        }
        query += QLatin1String("ORDER BY artist = '', artist, album = '', year");
        mQuery->prepare(query);
        if (!mAllArtists) {
            mQuery->addBindValue(mArtist);
        }

        execQuery();
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
    }

    const QString& AlbumsModel::artist() const
    {
        return mArtist;
    }

    void AlbumsModel::setArtist(const QString& artist)
    {
        mArtist = artist;
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
                {TracksCountRole, "tracksCount"},
                {DurationRole, "duration"}};
    }
}
