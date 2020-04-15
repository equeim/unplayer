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

#include "genresmodel.h"

#include <functional>

#include <QDebug>
#include <QFile>
#include <QFutureWatcher>
#include <QSqlError>
#include <QSqlQuery>
#include <QtConcurrentRun>

#include "libraryutils.h"
#include "modelutils.h"
#include "settings.h"

#include "abstractlibrarymodel.cpp"

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
        execQuery();
    }

    QVariant GenresModel::data(const QModelIndex& index, int role) const
    {
        const Genre& genre = mGenres[static_cast<size_t>(index.row())];
        switch (role) {
        case GenreRole:
            return genre.genre;
        case TracksCountRole:
            return genre.tracksCount;
        case DurationRole:
            return genre.duration;
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
        execQuery();
    }

    std::vector<LibraryTrack> GenresModel::getTracksForGenre(int index) const
    {
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT filePath, title, artist, album, duration, directoryMediaArt, embeddedMediaArt FROM tracks "
                                     "WHERE genre = ?  "
                                     "ORDER BY artist = '', artist, album = '', year, album, trackNumber, title"));
        query.addBindValue(mGenres[static_cast<size_t>(index)].genre);
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

        qWarning() << "failed to get tracks from database";
        return {};
    }

    std::vector<LibraryTrack> GenresModel::getTracksForGenres(const std::vector<int>& indexes) const
    {
        std::vector<LibraryTrack> tracks;
        QSqlDatabase::database().transaction();
        for (int index : indexes) {
            std::vector<LibraryTrack> genreTracks(getTracksForGenre(index));
            tracks.insert(tracks.end(), std::make_move_iterator(genreTracks.begin()), std::make_move_iterator(genreTracks.end()));
        }
        QSqlDatabase::database().commit();
        return tracks;
    }

    QStringList GenresModel::getTrackPathsForGenre(int index) const
    {
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT filePath FROM tracks "
                                     "WHERE genre = ?  "
                                     "ORDER BY artist = '', artist, album = '', year, album, trackNumber, title"));
        query.addBindValue(mGenres[static_cast<size_t>(index)].genre);
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

        qWarning() << "failed to get tracks from database";
        return {};
    }

    QStringList GenresModel::getTrackPathsForGenres(const std::vector<int>& indexes) const
    {
        QStringList tracks;
        QSqlDatabase::database().transaction();
        for (int index : indexes) {
            tracks.append(getTrackPathsForGenre(index));
        }
        QSqlDatabase::database().commit();
        return tracks;
    }

    void GenresModel::removeGenre(int index, bool deleteFiles)
    {
        removeGenres({index}, deleteFiles);
    }

    void GenresModel::removeGenres(const std::vector<int>& indexes, bool deleteFiles)
    {
        if (LibraryUtils::instance()->isRemovingFiles()) {
            return;
        }

        std::vector<QString> genres;
        genres.reserve(indexes.size());
        for (int index : indexes) {
            genres.push_back(mGenres[static_cast<size_t>(index)].genre);
        }
        LibraryUtils::instance()->removeArtists(std::move(genres), deleteFiles);
        QObject::connect(LibraryUtils::instance(), &LibraryUtils::removingFilesChanged, this, [this, indexes] {
            if (!LibraryUtils::instance()->isRemovingFiles()) {
                ModelBatchRemover::removeIndexes(this, indexes);
                QObject::disconnect(LibraryUtils::instance(), &LibraryUtils::removingFilesChanged, this, nullptr);
            }
        });
    }

    QHash<int, QByteArray> GenresModel::roleNames() const
    {
        return {{GenreRole, "genre"},
                {TracksCountRole, "tracksCount"},
                {DurationRole, "duration"}};
    }

    QString GenresModel::makeQueryString(std::vector<QVariant>&)
    {
        return QString::fromLatin1("SELECT genre, COUNT(*), SUM(duration) FROM tracks "
                                            "WHERE genre != '' "
                                            "GROUP BY genre "
                                            "ORDER BY genre %1").arg(mSortDescending ? QLatin1String("DESC")
                                                                                     : QLatin1String("ASC"));
    }

    Genre GenresModel::itemFromQuery(const QSqlQuery& query)
    {
        return {query.value(GenreField).toString(),
                query.value(TracksCountField).toInt(),
                query.value(DurationField).toInt()};
    }
}
