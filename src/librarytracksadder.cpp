 /*
 * Unplayer
 * Copyright (C) 2015-2020 Alexey Rochev <equeim@gmail.com>
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

#include "librarytracksadder.h"

#include <algorithm>

#include <QDebug>
#include <QVariant>
#include <QSqlError>
#include <QStringBuilder>

#include "sqlutils.h"
#include "tagutils.h"

namespace unplayer
{
    LibraryTracksAdder::LibraryTracksAdder(const QSqlDatabase& db)
        : mDb(db)
    {
        QSqlQuery query(db);
        if (query.exec(QLatin1String("SELECT id FROM tracks ORDER BY id DESC LIMIT 1"))) {
            if (query.next()) {
                mLastTrackId = query.value(0).toInt();
            } else {
                mLastTrackId = 0;
            }
        } else {
            qWarning() << "Failed to get last track id" << query.lastError();
        }

        getArtists();
        getAlbums();
        getGenres();

        if (!mAddTrackQuery.prepare(QLatin1String("INSERT INTO tracks (modificationTime, year, trackNumber, duration, filePath, title, discNumber, directoryMediaArt, embeddedMediaArt) "
                                                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"))) {
            qWarning() << "Failed to prepare new track query" << mAddTrackQuery.lastError();
        }
    }

    void LibraryTracksAdder::addTrackToDatabase(const QString& filePath,
                                                long long modificationTime,
                                                tagutils::Info& info,
                                                const QString& directoryMediaArt,
                                                const QString& embeddedMediaArt)
    {
        mAddTrackQuery.addBindValue(modificationTime);
        mAddTrackQuery.addBindValue(info.year);
        mAddTrackQuery.addBindValue(info.trackNumber);
        mAddTrackQuery.addBindValue(info.duration);
        mAddTrackQuery.addBindValue(filePath);
        mAddTrackQuery.addBindValue(info.title);
        mAddTrackQuery.addBindValue(nullIfEmpty(info.discNumber));
        mAddTrackQuery.addBindValue(nullIfEmpty(directoryMediaArt));
        mAddTrackQuery.addBindValue(nullIfEmpty(embeddedMediaArt));

        if (!mAddTrackQuery.exec()) {
            qWarning() << "Failed to insert track in the database" << mAddTrackQuery.lastError();
            return;
        }

        const int trackId = ++mLastTrackId;

        info.artists.append(info.albumArtists);
        info.artists.removeDuplicates();

        for (const QString& artist : info.artists) {
            const int artistId = getArtistId(artist);
            if (artistId != 0) {
                addRelationship(trackId, artistId, mArtists.addTracksRelationshipQuery);
            }
        }

        if (!info.albums.isEmpty()) {
            if (info.albumArtists.isEmpty() && !info.artists.isEmpty()) {
                info.albumArtists.push_back(info.artists.front());
            }

            QVector<int> artistIds;
            artistIds.reserve(info.albumArtists.size());
            for (const QString& albumArtist : info.albumArtists) {
                const int artistId = getArtistId(albumArtist);
                if (artistId != 0) {
                    artistIds.push_back(artistId);
                }
            }
            std::sort(artistIds.begin(), artistIds.end());

            for (const QString& album : info.albums) {
                const int albumId = getAlbumId(album, std::move(artistIds));
                if (albumId != 0) {
                    addRelationship(trackId, albumId, mAlbums.addTracksRelationshipQuery);
                }
            }
        }

        for (const QString& genre : info.genres) {
            const int genreId = getGenreId(genre);
            if (genreId != 0) {
                addRelationship(trackId, genreId, mGenres.addTracksRelationshipQuery);
            }
        }
    }

    int LibraryTracksAdder::getAddedArtistId(const QString& title)
    {
        if (title.isEmpty()) {
            return 0;
        }
        const auto& map = mArtists.ids;
        const auto found(map.find(title));
        return (found == map.end()) ? 0 : found->second;
    }

    int LibraryTracksAdder::getAddedAlbumId(const QString& title, const QVector<int>& artistIds)
    {
        if (title.isEmpty()) {
            return 0;
        }
        const auto& map = mAlbums.ids;
        const auto found(map.find(QPair<QString, QVector<int>>(title, artistIds)));
        return (found == map.end()) ? 0 : found->second;
    }

    void LibraryTracksAdder::getArtists()
    {
        getArtistsOrGenres(mArtists);
    }

    void LibraryTracksAdder::getGenres()
    {
        getArtistsOrGenres(mGenres);
    }

    void LibraryTracksAdder::getArtistsOrGenres(LibraryTracksAdder::ArtistsOrGenres& ids)
    {
        QSqlQuery query(mDb);
        if (query.exec(QLatin1String("SELECT id, title FROM ") % ids.table)) {
            if (reserveFromQuery(ids.ids, query) > 0) {
                while (query.next()) {
                    ids.lastId = query.value(0).toInt();
                    ids.ids.emplace(query.value(1).toString(), ids.lastId);
                }
            }
        } else {
            qWarning() << "Failed to exec query" << query.lastError();
        }
    }

    void LibraryTracksAdder::getAlbums()
    {
        enum
        {
            IdField,
            TitleField,
            ArtistIdField
        };
        QSqlQuery query(mDb);
        if (query.exec(QLatin1String("SELECT id, title, artistId "
                                     "FROM albums "
                                     "LEFT JOIN albums_artists ON albums_artists.albumId = albums.id"))) {
            QString title;
            QVector<int> artistIds;

            const auto addAlbum = [&] {
                std::sort(artistIds.begin(), artistIds.end());
                mAlbums.ids.emplace(QPair<QString, QVector<int>>(title, artistIds), mAlbums.lastId);
                artistIds.clear();
            };

            while (query.next()) {
                const int id = query.value(IdField).toInt();
                if (id != mAlbums.lastId) {
                    if (mAlbums.lastId != 0) {
                        addAlbum();
                    }
                    mAlbums.lastId = id;
                    title = query.value(TitleField).toString();
                }
                const int artistId = query.value(ArtistIdField).toInt();
                if (artistId != 0) {
                    artistIds.push_back(artistId);
                }
            }

            if (mAlbums.lastId != 0) {
                addAlbum();
            }
        } else {
            qWarning() << "Failed to exec query" << query.lastError();
        }
    }

    int LibraryTracksAdder::getArtistId(const QString& title)
    {
        return getArtistOrGenreId(title, mArtists);
    }

    int LibraryTracksAdder::getGenreId(const QString& title)
    {
        return getArtistOrGenreId(title, mGenres);
    }

    int LibraryTracksAdder::getArtistOrGenreId(const QString& title, LibraryTracksAdder::ArtistsOrGenres& ids)
    {
        if (title.isEmpty()) {
            return 0;
        }
        const auto& map = ids.ids;
        const auto found(map.find(title));
        if (found == map.end()) {
            return addArtistOrGenre(title, ids);
        }
        return found->second;
    }

    int LibraryTracksAdder::addArtistOrGenre(const QString& title, LibraryTracksAdder::ArtistsOrGenres& ids)
    {
        ids.addNewQuery.addBindValue(title);
        if (!ids.addNewQuery.exec()) {
            qWarning() << "Failed to exec query" << ids.addNewQuery.lastError();
            return 0;
        }
        ++ids.lastId;
        ids.ids.emplace(title, ids.lastId);
        return ids.lastId;
    }

    int LibraryTracksAdder::getAlbumId(const QString& title, QVector<int>&& artistIds)
    {
        if (title.isEmpty()) {
            return 0;
        }
        const auto& map = mAlbums.ids;
        const auto found(map.find(QPair<QString, QVector<int>>(title, artistIds)));
        if (found == map.end()) {
            return addAlbum(title, std::move(artistIds));
        }
        return found->second;
    }

    int LibraryTracksAdder::addAlbum(const QString& title, QVector<int>&& artistIds)
    {
        mAlbums.addNewQuery.addBindValue(title);
        if (!mAlbums.addNewQuery.exec()) {
            qWarning() << "Failed to exec query" << mAlbums.addNewQuery.lastError();
            return 0;
        }

        ++mAlbums.lastId;

        for (int artistId : artistIds) {
            addRelationship(mAlbums.lastId, artistId, mAlbums.addArtistsRelationshipQuery);
        }

        mAlbums.ids.emplace(QPair<QString, QVector<int>>(title, std::move(artistIds)), mAlbums.lastId);
        return mAlbums.lastId;
    }

    void LibraryTracksAdder::addRelationship(int firstId, int secondId, QSqlQuery& query)
    {
        query.addBindValue(firstId);
        query.addBindValue(secondId);
        if (!query.exec()) {
            qWarning() << "Failed to exec query" << query.lastError();
        }
    }

    LibraryTracksAdder::ArtistsOrGenres::ArtistsOrGenres(QLatin1String table, const QSqlDatabase& db)
        : table(table),
          addNewQuery(db),
          addTracksRelationshipQuery(db)
    {
        if (!addNewQuery.prepare(QString::fromLatin1("INSERT INTO %1 (title) VALUES (?)").arg(table))) {
            qWarning() << "Failed to prepare query" << addNewQuery.lastError();
        }
        if (!addTracksRelationshipQuery.prepare(QString::fromLatin1("INSERT INTO tracks_%1 VALUES (?, ?)").arg(table))) {
            qWarning() << "Failed to prepare query" << addTracksRelationshipQuery.lastError();
        }
    }

    LibraryTracksAdder::Albums::Albums(const QSqlDatabase& db)
        : addNewQuery(db),
          addTracksRelationshipQuery(db),
          addArtistsRelationshipQuery(db)
    {
        if (!addNewQuery.prepare(QLatin1String("INSERT INTO albums (title) VALUES (?)"))) {
            qWarning() << "Failed to prepare query" << addNewQuery.lastError();
        }
        if (!addTracksRelationshipQuery.prepare(QLatin1String("INSERT INTO tracks_albums VALUES (?, ?)"))) {
            qWarning() << "Failed to prepare query" << addTracksRelationshipQuery.lastError();
        }
        if (!addArtistsRelationshipQuery.prepare(QLatin1String("INSERT INTO albums_artists VALUES (?, ?)"))) {
            qWarning() << "Failed to prepare query" << addArtistsRelationshipQuery.lastError();
        }
    }
}
