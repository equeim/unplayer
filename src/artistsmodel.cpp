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

#include "artistsmodel.h"

#include <functional>

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFutureWatcher>
#include <QSqlError>
#include <QSqlQuery>
#include <QtConcurrentRun>

#include "libraryutils.h"
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
        execQuery();
    }

    QVariant ArtistsModel::data(const QModelIndex& index, int role) const
    {
        const Artist& artist = mArtists[index.row()];

        switch (role) {
        case ArtistRole:
            return artist.artist;
        case DisplayedArtistRole:
            return artist.displayedArtist;
        case AlbumsCountRole:
            return artist.albumsCount;
        case TracksCountRole:
            return artist.tracksCount;
        case DurationRole:
            return artist.duration;
        default:
            return QVariant();
        }
    }

    int ArtistsModel::rowCount(const QModelIndex&) const
    {
        return mArtists.size();
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
        execQuery();
    }

    std::vector<LibraryTrack> ArtistsModel::getTracksForArtist(int index) const
    {
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT filePath, title, artist, album, duration, directoryMediaArt, embeddedMediaArt FROM tracks "
                                     "WHERE artist = ? "
                                     "ORDER BY album = '', year, album, trackNumber, title"));
        query.addBindValue(mArtists[index].artist);
        if (query.exec()) {
            std::vector<LibraryTrack> tracks;
            query.last();
            if (query.at() >= 0) {
                tracks.reserve(query.at() + 1);
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

    std::vector<LibraryTrack> ArtistsModel::getTracksForArtists(const std::vector<int>& indexes) const
    {
        std::vector<LibraryTrack> tracks;
        QSqlDatabase::database().transaction();
        for (int index : indexes) {
            std::vector<LibraryTrack> artistTracks(getTracksForArtist(index));
            tracks.insert(tracks.end(), std::make_move_iterator(artistTracks.begin()), std::make_move_iterator(artistTracks.end()));
        }
        QSqlDatabase::database().commit();
        return tracks;
    }

    QStringList ArtistsModel::getTrackPathsForArtist(int index) const
    {
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT filePath FROM tracks "
                                     "WHERE artist = ? "
                                     "ORDER BY album = '', year, album, trackNumber, title"));
        query.addBindValue(mArtists[index].artist);
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

    QStringList ArtistsModel::getTrackPathsForArtists(const std::vector<int>& indexes) const
    {
        QStringList tracks;
        QSqlDatabase::database().transaction();
        for (int index : indexes) {
            tracks.append(getTrackPathsForArtist(index));
        }
        QSqlDatabase::database().commit();
        return tracks;
    }

    void ArtistsModel::removeArtist(int index, bool deleteFiles)
    {
        removeArtists({index}, deleteFiles);
    }

    void ArtistsModel::removeArtists(std::vector<int> indexes, bool deleteFiles)
    {
        if (LibraryUtils::instance()->isRemovingFiles()) {
            return;
        }

        std::vector<QString> artists;
        artists.reserve(indexes.size());
        for (int index : indexes) {
            artists.push_back(mArtists[index].artist);
        }
        QObject::connect(LibraryUtils::instance(), &LibraryUtils::removingFilesChanged, this, std::bind([this](std::vector<int>& indexes) {
            if (!LibraryUtils::instance()->isRemovingFiles()) {
                for (int i = indexes.size() - 1; i >= 0; --i) {
                    const int index = indexes[i];
                    beginRemoveRows(QModelIndex(), index, index);
                    mArtists.erase(mArtists.begin() + index);
                    endRemoveRows();
                }
                QObject::disconnect(LibraryUtils::instance(), &LibraryUtils::removingFilesChanged, this, nullptr);
            }
        }, std::move(indexes)));
        LibraryUtils::instance()->removeArtists(std::move(artists), deleteFiles);
    }

    QHash<int, QByteArray> ArtistsModel::roleNames() const
    {
        return {{ArtistRole, "artist"},
                {DisplayedArtistRole, "displayedArtist"},
                {AlbumsCountRole, "albumsCount"},
                {TracksCountRole, "tracksCount"},
            {DurationRole, "duration"}};
    }

    void ArtistsModel::execQuery()
    {
        beginResetModel();
        mArtists.clear();
        QSqlQuery query(QString::fromLatin1("SELECT artist, COUNT(DISTINCT(album)), COUNT(*), SUM(duration) FROM "
                                            "(SELECT artist, album, duration FROM tracks GROUP BY id, artist, album) "
                                            "GROUP BY artist "
                                            "ORDER BY artist = '' %1, artist %1").arg(mSortDescending ? QLatin1String("DESC")
                                                                                                      : QLatin1String("ASC")));
        if (query.lastError().type() == QSqlError::NoError) {
            while (query.next()) {
                const QString artist(query.value(ArtistField).toString());
                mArtists.push_back({artist,
                                    artist.isEmpty() ? qApp->translate("unplayer", "Unknown artist") : artist,
                                    query.value(AlbumsCountField).toInt(),
                                    query.value(TracksCountField).toInt(),
                                    query.value(DurationField).toInt()});
            }

        } else {
            qWarning() << query.lastError();
        }
        endResetModel();
    }
}
