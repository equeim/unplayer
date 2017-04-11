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

#include "artistsmodel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QSqlError>

#include "settings.h"

namespace unplayer
{
    namespace
    {
        enum Field
        {
            ArtistField,
            AlbumsCountField,
            TracksCountField,
            DurationField
        };
    }

    ArtistsModel::ArtistsModel()
        : mSortDescending(Settings::instance()->artistsSortDescending())
    {
        setQuery();
    }

    QVariant ArtistsModel::data(const QModelIndex& index, int role) const
    {
        mQuery->seek(index.row());

        switch (role) {
        case ArtistRole:
            return mQuery->value(ArtistField).toString();
        case DisplayedArtistRole:
        {
            const QString artist(mQuery->value(ArtistField).toString());
            if (artist.isEmpty()) {
                return qApp->translate("unplayer", "Unknown artist");
            }
            return artist;
        }
        case AlbumsCountRole:
            return mQuery->value(AlbumsCountField);
        case TracksCountRole:
            return mQuery->value(TracksCountField);
        case DurationRole:
            return mQuery->value(DurationField);
        default:
            return QVariant();
        }
    }

    bool ArtistsModel::sortDescending() const
    {
        return mSortDescending;
    }

    void ArtistsModel::toggleSortOrder()
    {
        mSortDescending = !mSortDescending;
        Settings::instance()->setArtistsSortDescending(mSortDescending);
        emit sortDescendingChanged();
        setQuery();
    }

    QStringList ArtistsModel::getTracksForArtist(int index) const
    {
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT filePath FROM tracks "
                                     "WHERE artist = ? "
                                     "ORDER BY album = '', year, album, trackNumber, title"));
        mQuery->seek(index);
        query.addBindValue(mQuery->value(ArtistField).toString());
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

    QStringList ArtistsModel::getTracksForArtists(const QVector<int>& indexes) const
    {
        QStringList tracks;
        QSqlDatabase::database().transaction();
        for (int index : indexes) {
            tracks.append(getTracksForArtist(index));
        }
        QSqlDatabase::database().commit();
        return tracks;
    }

    QHash<int, QByteArray> ArtistsModel::roleNames() const
    {
        return {{ArtistRole, "artist"},
                {DisplayedArtistRole, "displayedArtist"},
                {AlbumsCountRole, "albumsCount"},
                {TracksCountRole, "tracksCount"},
                {DurationRole, "duration"}};
    }

    void ArtistsModel::setQuery()
    {
        QString query(QLatin1String("SELECT artist, COUNT(DISTINCT(album)), COUNT(*), SUM(duration) FROM "
                                    "(SELECT artist, album, duration FROM tracks GROUP BY filePath, artist) "
                                    "GROUP BY artist "
                                    "ORDER BY artist = '', artist"));
        if (mSortDescending) {
            query += QLatin1String(" DESC");
        }

        beginResetModel();
        mQuery->prepare(query);
        execQuery();
        endResetModel();
    }
}
