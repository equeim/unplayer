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

#include <QDebug>
#include <QVariant>
#include <QSqlError>

#include "sqlutils.h"
#include "tagutils.h"

namespace unplayer
{
    LibraryTracksAdder::LibraryTracksAdder(const QSqlDatabase& db)
        : mDb(db)
    {
        if (mQuery.exec(QLatin1String("SELECT id FROM tracks ORDER BY id DESC LIMIT 1"))) {
            if (mQuery.next()) {
                mLastTrackId = mQuery.value(0).toInt();
            } else {
                mLastTrackId = 0;
            }
        } else {
            qWarning() << __func__ << "failed to get last track id" << mQuery.lastError();
        }

        getArtists();
        getAlbums();
        getGenres();
    }

    void LibraryTracksAdder::addTrackToDatabase(const QString& filePath,
                                                long long modificationTime,
                                                tagutils::Info& info,
                                                const QString& directoryMediaArt,
                                                const QString& embeddedMediaArt)
    {
        mQuery.prepare([&]() {
            QString queryString(QStringLiteral("INSERT INTO tracks (modificationTime, year, trackNumber, duration, filePath, title, discNumber, directoryMediaArt, embeddedMediaArt) "
                                               "VALUES (%1, %2, %3, %4, ?, ?, ?, ?, ?)"));
            queryString = queryString.arg(modificationTime).arg(info.year).arg(info.trackNumber).arg(info.duration);
            return queryString;
        }());

        mQuery.addBindValue(filePath);
        mQuery.addBindValue(info.title);
        mQuery.addBindValue(nullIfEmpty(info.discNumber));
        mQuery.addBindValue(nullIfEmpty(directoryMediaArt));
        mQuery.addBindValue(nullIfEmpty(embeddedMediaArt));

        if (!mQuery.exec()) {
            qWarning() << __func__ << "failed to insert track in the database" << mQuery.lastError();
            return;
        }

        const int trackId = ++mLastTrackId;

        info.artists.append(info.albumArtists);
        info.artists.removeDuplicates();

        for (const QString& artist : info.artists) {
            const int artistId = getArtistId(artist);
            if (artistId != 0) {
                addRelationship(trackId, artistId, "tracks_artists");
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
                    addRelationship(trackId, albumId, "tracks_albums");
                }
            }
        }

        for (const QString& genre : info.genres) {
            const int genreId = getGenreId(genre);
            if (genreId != 0) {
                addRelationship(trackId, genreId, "tracks_genres");
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

    void LibraryTracksAdder::getAlbums()
    {
        enum
        {
            IdField,
            TitleField,
            ArtistIdField
        };
        if (mQuery.exec(QLatin1String("SELECT id, title, artistId "
                                      "FROM albums "
                                      "LEFT JOIN albums_artists ON albums_artists.albumId = albums.id"))) {
            QVector<int> artistIds;
            while (mQuery.next()) {
                {
                    const int artistId = mQuery.value(ArtistIdField).toInt();
                    if (artistId != 0) {
                        artistIds.push_back(artistId);
                    }
                }
                const int id = mQuery.value(IdField).toInt();
                if (id != mAlbums.lastId && mAlbums.lastId != 0) {
                    std::sort(artistIds.begin(), artistIds.end());
                    mAlbums.ids.emplace(QPair<QString, QVector<int>>(mQuery.value(TitleField).toString(), artistIds), mAlbums.lastId);
                    artistIds.clear();
                }
                mAlbums.lastId = id;
            }
        } else {
            qWarning() << __func__ << mQuery.lastError();
        }
    }

    void LibraryTracksAdder::getGenres()
    {
        getArtistsOrGenres(mGenres);
    }

    int LibraryTracksAdder::getArtistId(const QString& title)
    {
        return getArtistOrGenreId(title, mArtists);
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

    int LibraryTracksAdder::getGenreId(const QString& title)
    {
        return getArtistOrGenreId(title, mGenres);
    }

    void LibraryTracksAdder::addRelationship(int firstId, int secondId, const char* table)
    {
        /*if (!mQuery.prepare()) {
            qWarning() << __func__ << "failed to prepare query" << mQuery.lastError();
        }*/
        if (!mQuery.exec(QStringLiteral("INSERT INTO %1 VALUES (%2, %3)").arg(QLatin1String(table)).arg(firstId).arg(secondId))) {
            qWarning() << __func__ << "failed to exec query" << mQuery.lastError();
        }
    }

    int LibraryTracksAdder::addAlbum(const QString& title, QVector<int>&& artistIds)
    {
        if (!mQuery.prepare(QStringLiteral("INSERT INTO albums (title) VALUES (?)"))) {
            qWarning() << __func__ << "failed to prepare query" << mQuery.lastError();
            return 0;
        }
        mQuery.addBindValue(title);
        if (!mQuery.exec()) {
            qWarning() << __func__ << "failed to exec query" << mQuery.lastError();
            return 0;
        }

        ++mAlbums.lastId;

        for (int artistId : artistIds) {
            addRelationship(mAlbums.lastId, artistId, "albums_artists");
        }

        mAlbums.ids.emplace(QPair<QString, QVector<int>>(title, std::move(artistIds)), mAlbums.lastId);
        return mAlbums.lastId;
    }

    void LibraryTracksAdder::getArtistsOrGenres(LibraryTracksAdder::ArtistsOrGenres& ids)
    {
        QString queryString(QLatin1String("SELECT id, title FROM "));
        queryString.push_back(ids.table);
        if (mQuery.exec(queryString)) {
            if (reserveFromQuery(ids.ids, mQuery) > 0) {
                while (mQuery.next()) {
                    ids.lastId = mQuery.value(0).toInt();
                    ids.ids.emplace(mQuery.value(1).toString(), ids.lastId);
                }
            }
        } else {
            qWarning() << __func__ << mQuery.lastError();
        }
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
        if (!mQuery.prepare(QStringLiteral("INSERT INTO %1 (title) VALUES (?)").arg(ids.table))) {
            qWarning() << __func__ << "failed to prepare query" << mQuery.lastError();
            return 0;
        }
        mQuery.addBindValue(title);
        if (!mQuery.exec()) {
            qWarning() << __func__ << "failed to exec query" << mQuery.lastError();
            return 0;
        }
        ++ids.lastId;
        ids.ids.emplace(title, ids.lastId);
        return ids.lastId;
    }
}
