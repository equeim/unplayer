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

#ifndef UNPLAYER_TRACKSQUERY_H
#define UNPLAYER_TRACKSQUERY_H

#include <algorithm>
#include <set>
#include <unordered_map>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>

#include "libraryutils.h"
#include "stdutils.h"
#include "sqlutils.h"
#include "utilsfunctions.h"

namespace unplayer
{
    inline QString joinStrings(const QStringList& strings)
    {
        return strings.join(QLatin1String(", "));
    }

    template<typename Track>
    bool queryTracksByPaths(std::set<QString> tracksToQuery, std::unordered_multimap<QString, Track>& tracksToQueryMap, QLatin1String dbConnectionName)
    {
        if (tracksToQuery.empty()) {
            return true;
        }

        qInfo("Start querying tracks from db, count=%zu", tracksToQuery.size());

        QElapsedTimer timer;
        timer.start();

        const DatabaseConnectionGuard databaseGuard{dbConnectionName};
        QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
        if (!db.isOpen()) {
            return false;
        }

        bool abort = false;

        QSqlQuery query(db);
        size_t previousCount = 0;

        auto tracksIter(tracksToQuery.begin());

        batchedCount(tracksToQuery.size(), LibraryUtils::maxDbVariableCount, [&](size_t, size_t count) {
            enum {
                FilePathField,
                TitleField,
                DurationField,
                ArtistField,
                AlbumField
            };

            if (!qApp) {
                abort = true;
            }

            if (abort) {
                return;
            }

            if (count != previousCount) {
                QString queryString(QLatin1String("SELECT filePath, tracks.title, duration, artists.title, albums.title "
                                                  "FROM tracks "
                                                  "LEFT JOIN tracks_artists ON tracks_artists.trackId = tracks.id "
                                                  "LEFT JOIN artists ON artists.id = tracks_artists.artistId "
                                                  "LEFT JOIN tracks_albums ON tracks_albums.trackId = tracks.id "
                                                  "LEFT JOIN albums ON albums.id = tracks_albums.albumId "
                                                  "WHERE filePath IN ("));
                queryString += makeInStringForParameters(count);

                if (!query.prepare(queryString)) {
                    qWarning() << query.lastError();
                    abort = true;
                    return;
                }

                previousCount = count;
            }

            for (size_t i = 0; i < count; ++i) {
                query.addBindValue(*tracksIter);
                ++tracksIter;
            }

            if (!query.exec()) {
                qWarning() << query.lastError();
                abort = true;
                return;
            }

            QString filePath;
            QStringList artists;
            QStringList albums;

            const auto insert = [&] {
                const auto found(tracksToQueryMap.equal_range(filePath));
                if (found.first != tracksToQueryMap.end()) {
                    QString title(query.value(TitleField).toString());
                    if (title.isEmpty()) {
                        title = QFileInfo(filePath).fileName();
                    }
                    const QString artist(joinStrings(artists));
                    const QString album(joinStrings(albums));

                    for (auto i = found.first; i != found.second; ++i) {
                        Track& track = i->second;
                        track->title = title;
                        track->duration = query.value(DurationField).toInt();
                        track->artist = artist;
                        track->album = album;
                    }
                    tracksToQueryMap.erase(found.first, found.second);
                }
                artists.clear();
                albums.clear();
            };

            while (query.next()) {
                QString newFilePath(query.value(FilePathField).toString());
                if (newFilePath != filePath) {
                    if (!filePath.isEmpty()) {
                        insert();
                    }
                    filePath = std::move(newFilePath);
                }
                const QString artist(query.value(ArtistField).toString());
                if (!artist.isEmpty() && !artists.contains(artist)) {
                    artists.push_back(artist);
                }
                const QString album(query.value(AlbumField).toString());
                if (!album.isEmpty() && !albums.contains(album)) {
                    albums.push_back(album);
                }
            }
            if (!filePath.isEmpty()) {
                query.previous();
                insert();
            }

            query.finish();
        });

        qInfo("Finished querying tracks: %lldms", static_cast<long long>(timer.elapsed()));

        return !abort;
    }
}

#endif // UNPLAYER_TRACKSQUERY_H
