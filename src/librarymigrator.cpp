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

#include "librarymigrator.h"

#include <algorithm>
#include <vector>

#include <QDebug>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlRecord>
#include <QVariant>
#include <QVector>

#include "librarytracksadder.h"
#include "libraryutils.h"
#include "stdutils.h"
#include "tagutils.h"
#include "utilsfunctions.h"

#include <QElapsedTimer>

namespace unplayer
{
    LibraryMigrator::LibraryMigrator(const QSqlDatabase& db)
        : mDb(db)
    {

    }

    bool LibraryMigrator::migrateIfNeeded(int latestVersion)
    {
        if (!mQuery.exec(QLatin1String("PRAGMA user_version"))) {
            qWarning() << "Failed to get user_version" << mQuery.lastError();
            return false;
        }
        mQuery.next();

        int currentVersion = mQuery.value(0).toInt();

        if (currentVersion == latestVersion) {
            return true;
        }

        if (currentVersion > latestVersion) {
            qWarning("Current database version is higher than latest version, abort");
            return false;
        }

        QElapsedTimer timer;
        timer.start();

        mDb.transaction();

        for (; currentVersion != latestVersion; ++currentVersion) {
            qInfo("Migrating from version %d", currentVersion);

            bool abort = false;

            switch (currentVersion) {
            case 0:
            {
                if (!migrateFrom0()) {
                    abort = true;
                }
                break;
            default:
                break;
            }
            }

            if (abort) {
                qWarning("Failed to migrate from version %d", currentVersion);
                mDb.rollback();
                return false;
            }

            qInfo("Migrated from version %d", currentVersion);
        }

        if (!mQuery.exec(QString::fromLatin1("PRAGMA user_version = %1").arg(latestVersion))) {
            qWarning() << "Failed to set user_version" << mQuery.lastError();
            mDb.rollback();
            return false;
        }

        mDb.commit();

        qInfo("Migration took %lld ms", static_cast<long long>(timer.elapsed()));

        return true;
    }

    bool LibraryMigrator::migrateFrom0()
    {
        {
            if (mDb.tables() != QStringList{QLatin1String("tracks")}) {
                qWarning("'tracks' table must be the only table in the database");
                return false;
            }

            const std::vector<QString> requiredColumns{
                QLatin1String("id"),
                QLatin1String("filePath"),
                QLatin1String("modificationTime"),
                QLatin1String("title"),
                QLatin1String("artist"),
                QLatin1String("albumArtist"),
                QLatin1String("album"),
                QLatin1String("year"),
                QLatin1String("trackNumber"),
                QLatin1String("discNumber"),
                QLatin1String("genre"),
                QLatin1String("duration"),
                QLatin1String("directoryMediaArt"),
                QLatin1String("embeddedMediaArt")
            };

            if (!mQuery.exec(QLatin1String("PRAGMA table_info(tracks)"))) {
                qWarning() << "Failed to get 'tracks' table info" << mQuery.lastError();
                return false;
            }
            std::vector<QString> columns;
            columns.reserve(requiredColumns.size());
            while (mQuery.next()) {
                columns.push_back(mQuery.value(1).toString());
            }
            mQuery.clear();

            if (columns != requiredColumns) {
                qWarning("Unexpected columns for 'tracks' table");
                return false;
            }
        }

        if (!mQuery.exec(QLatin1String("ALTER TABLE tracks RENAME TO tracks_old"))) {
            qWarning() << "Failed to rename 'tracks' table" << mQuery.lastError();
            return false;
        }

        if (!LibraryUtils::createTables(mDb)) {
            qWarning("Failed to create tables");
            return false;
        }

        std::unordered_map<int, QString> userMediaArtHash;
        if (!migrateOldTracks(userMediaArtHash)) {
            return false;
        }

        if (!mQuery.prepare(QLatin1String("UPDATE albums SET userMediaArt = ? WHERE id = ?"))) {
            qWarning() << "Failed to prepare user media art query" << mQuery.lastError();
        }

        for (const auto& i : userMediaArtHash) {
            const int albumId = i.first;
            const QString& userMediaArt = i.second;

            qInfo() << "Setting user media art" << userMediaArt << "for album id" << albumId;

            mQuery.addBindValue(userMediaArt);
            mQuery.addBindValue(albumId);
            if (!mQuery.exec()) {
                qWarning() << "Failed to set user media art" << mQuery.lastError();
                return false;
            }
            mQuery.finish();
        }

        if (!LibraryUtils::createIndexes(mDb)) {
            qWarning("Failed to create indexes");
            return false;
        }

        mQuery.clear();

        return true;
    }

    namespace
    {
        inline bool addIfNotEmpty(QStringList& list, const QString& string)
        {
            if (!string.isEmpty()) {
                list.push_back(string);
                return true;
            }
            return false;
        }
    }

    bool LibraryMigrator::migrateOldTracks(std::unordered_map<int, QString>& userMediaArtHash)
    {
        enum Fields
        {
            IdField,
            FilePathField,
            ModificationTimeField,
            TitleField,
            ArtistField,
            AlbumArtistField,
            AlbumField,
            YearField,
            TrackNumberField,
            DiscNumberField,
            GenreField,
            DurationField,
            DirectoryMediaArtField,
            EmbeddedMediaArtField
        };

        if (!mQuery.exec(QLatin1String("SELECT * FROM tracks_old ORDER BY id"))) {
            qWarning() << "Failed to get tracks" << mQuery.lastError();
            return false;
        }

        std::unordered_map<QString, QString> trackUserMediaArtHash;
        const QRegularExpression embeddedMediaArtRegex(QLatin1String("^.*\\/\\w+-embedded\\.\\w+$"));
        embeddedMediaArtRegex.optimize();

        LibraryTracksAdder adder(mDb);

        int id = -1;
        long long modificationTime = 0;
        tagutils::Info info{};
        QString directoryMediaArt;
        QString embeddedMediaArt;

        const auto insert = [&] {
            std::sort(info.artists.begin(), info.artists.end());
            auto newEnd = std::unique(info.artists.begin(), info.artists.end());
            info.artists.erase(newEnd, info.artists.end());

            std::sort(info.albumArtists.begin(), info.albumArtists.end());
            info.albumArtists.erase(std::unique(info.albumArtists.begin(), info.albumArtists.end()), info.albumArtists.end());

            if (info.artists.size() > 1 && info.artists == info.albums) {
                info.albumArtists.erase(info.albumArtists.begin() + 1, info.albumArtists.end());
            }

            info.albums.removeDuplicates();
            info.genres.removeDuplicates();

            adder.addTrackToDatabase(info.filePath, modificationTime, info, directoryMediaArt, embeddedMediaArt);         
            embeddedMediaArt.clear();

            if (!trackUserMediaArtHash.empty()) {
                QVector<int> albumArtists;
                albumArtists.reserve(info.albumArtists.size());
                for (const QString& albumArtist : info.albumArtists) {
                    const int artistId = adder.getAddedArtistId(albumArtist);
                    if (artistId != 0) {
                        albumArtists.push_back(artistId);
                    } else {
                        qWarning() << "No artist id for" << albumArtist;
                    }
                }

                for (auto& i : trackUserMediaArtHash) {
                    const int albumId = adder.getAddedAlbumId(i.first, albumArtists);
                    if (albumId != 0) {
                        userMediaArtHash.emplace(albumId, std::move(i.second));
                    } else {
                        qWarning() << "No album id for" << i.first;
                    }
                }

                trackUserMediaArtHash.clear();
            }

            info = {};
        };

        while (mQuery.next()) {
            const int newId = mQuery.value(IdField).toInt();
            if (newId != id) {
                if (id != -1) {
                    insert();
                }
                id = newId;
                modificationTime = mQuery.value(ModificationTimeField).toLongLong();
                info.filePath = mQuery.value(FilePathField).toString();
                info.title = mQuery.value(TitleField).toString();
                info.year = mQuery.value(YearField).toInt();
                info.trackNumber = mQuery.value(TrackNumberField).toInt();
                info.discNumber = mQuery.value(DiscNumberField).toString();
                info.duration = mQuery.value(DurationField).toInt();
                directoryMediaArt = mQuery.value(DirectoryMediaArtField).toString();
            }

            addIfNotEmpty(info.artists, mQuery.value(ArtistField).toString());
            addIfNotEmpty(info.albumArtists, mQuery.value(AlbumArtistField).toString());
            const bool addedAlbum = addIfNotEmpty(info.albums, mQuery.value(AlbumField).toString());
            addIfNotEmpty(info.genres, unquote(mQuery.value(GenreField).toString().trimmed()));

            QString newEmbeddedMediaArt(mQuery.value(EmbeddedMediaArtField).toString());
            if (!newEmbeddedMediaArt.isEmpty()) {
                if (embeddedMediaArtRegex.match(newEmbeddedMediaArt).hasMatch()) {
                    if (embeddedMediaArt.isEmpty()) {
                        embeddedMediaArt = std::move(newEmbeddedMediaArt);
                    }
                } else {
                    // User media art
                    if (addedAlbum) {
                        trackUserMediaArtHash.emplace(info.albums.back(), std::move(newEmbeddedMediaArt));
                    } else {
                        qWarning() << "Record has user media art but album is empty:";
                        qWarning() << mQuery.record();
                    }
                }
            }
        }

        if (id != -1) {
            insert();
        }

        if (!mQuery.exec(QLatin1String("DROP TABLE tracks_old"))) {
            qWarning() << "Failed to remove table" << mQuery.lastError();
            return false;
        }

        mQuery.clear();

        return true;
    }
}
