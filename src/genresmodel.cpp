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

#include "genresmodel.h"

#include <QDebug>

#include "settings.h"

namespace unplayer
{
    namespace
    {
        enum Field
        {
            GenreField,
            TracksCountField,
            DurationField
        };

        enum Role
        {
            GenreRole = Qt::UserRole,
            TracksCountRole,
            DurationRole
        };
    }

    GenresModel::GenresModel()
        : mSortDescending(Settings::instance()->genresSortDescending())
    {
        setQuery();
    }

    QVariant GenresModel::data(const QModelIndex& index, int role) const
    {
        mQuery->seek(index.row());
        switch (role) {
        case GenreRole:
            return mQuery->value(GenreField);
        case TracksCountRole:
            return mQuery->value(TracksCountField);
        case DurationRole:
            return mQuery->value(DurationField);
        default:
            return QVariant();
        }
    }

    bool GenresModel::sortDescending() const
    {
        return mSortDescending;
    }

    void GenresModel::toggleSortOrder()
    {
        mSortDescending = !mSortDescending;
        Settings::instance()->setGenresSortDescending(mSortDescending);
        emit sortDescendingChanged();
        setQuery();
    }

    QStringList GenresModel::getTracksForGenre(int index) const
    {
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT filePath FROM tracks "
                                     "WHERE genre = ?  "
                                     "ORDER BY artist = '', artist, album = '', year, album, trackNumber, title"));
        mQuery->seek(index);
        query.addBindValue(mQuery->value(GenreField).toString());
        if (query.exec()) {
            QStringList tracks;
            while (query.next()) {
                tracks.append(query.value(0).toString());
            }
            return tracks;
        }

        qWarning() << "failed to get tracks from database";
        return QStringList();
    }

    QStringList GenresModel::getTracksForGenres(const QVector<int>& indexes) const
    {
        QStringList tracks;
        QSqlDatabase::database().transaction();
        for (int index : indexes) {
            tracks.append(getTracksForGenre(index));
        }
        QSqlDatabase::database().commit();
        return tracks;
    }

    QHash<int, QByteArray> GenresModel::roleNames() const
    {
        return {{GenreRole, "genre"},
                {TracksCountRole, "tracksCount"},
                {DurationRole, "duration"}};
    }

    void GenresModel::setQuery()
    {
        beginResetModel();
        mQuery->prepare(QString::fromLatin1("SELECT genre, COUNT(*), SUM(duration) FROM tracks "
                                            "WHERE genre != '' "
                                            "GROUP BY genre "
                                            "ORDER BY genre %1").arg(mSortDescending ? QLatin1String("DESC")
                                                                                     : QLatin1String("ASC")));
        execQuery();
        endResetModel();
    }
}
