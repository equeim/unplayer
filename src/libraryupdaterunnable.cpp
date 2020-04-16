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

#include "libraryupdaterunnable.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QMimeDatabase>
#include <QStringBuilder>
#include <QSqlError>

#include "settings.h"
#include "tagutils.h"
#include "utilsfunctions.h"

namespace unplayer
{
    namespace
    {
        const QLatin1String rescanConnectionName("unplayer_rescan");

        inline QVariant nullIfEmpty(const QString& string)
        {
            if (string.isEmpty()) {
                return {};
            }
            return string;
        }
    }

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
        if (!mQuery.prepare(QStringLiteral("INSERT INTO %1 VALUES (%2, %3)").arg(QLatin1String(table)).arg(firstId).arg(secondId))) {
            qWarning() << __func__ << "failed to prepare query" << mQuery.lastError();
        }
        if (!mQuery.exec()) {
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
            if (mQuery.last()) {
                ids.ids.reserve(static_cast<size_t>(mQuery.at() + 1));
                mQuery.seek(QSql::BeforeFirstRow);
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

    LibraryUpdateRunnable::LibraryUpdateRunnable(const QString& mediaArtDirectory)
        : mMediaArtDirectory(mediaArtDirectory),
          mCancel(false)
    {

    }

    LibraryUpdateRunnableNotifier* LibraryUpdateRunnable::notifier()
    {
        return &mNotifier;
    }

    void LibraryUpdateRunnable::cancel()
    {
        qInfo("Cancel updating database");
        mCancel = true;
    }

    void LibraryUpdateRunnable::run()
    {
        const struct FinishedGuard
        {
            ~FinishedGuard() {
                emit notifier.finished();
            }
            LibraryUpdateRunnableNotifier& notifier;
        } finishedGuard{mNotifier};

        if (mCancel) {
            return;
        }

        qInfo("Start updating database");
        QElapsedTimer timer;
        timer.start();
        QElapsedTimer stageTimer;
        stageTimer.start();

        const DatabaseGuard databaseGuard{rescanConnectionName};

        // Open database
        QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
        if (!db.isOpen()) {
            return;
        }
        db.transaction();

        const CommitGuard commitGuard{db};

        // Create media art directory
        if (!QDir().mkpath(mMediaArtDirectory)) {
            qWarning() << "failed to create media art directory:" << mMediaArtDirectory;
        }
        std::unordered_map<QByteArray, QString> embeddedMediaArtFiles(LibraryUtils::instance()->getEmbeddedMediaArt());

        struct FileToAdd
        {
            QString filePath;
            QString directoryMediaArt;
            Extension extension;
        };
        std::vector<FileToAdd> filesToAdd;

        const QMimeDatabase mimeDb;

        {
            // Library directories
            const auto prepareDirs = [](QStringList&& dirs) {
                dirs.removeDuplicates();
                for (QString& dir : dirs) {
                    if (!dir.endsWith(QLatin1Char('/'))) {
                        dir.push_back(QLatin1Char('/'));
                    }
                }

                for (int i = 0, max = dirs.size(); i < max; ++i) {
                    const QString& dir = dirs[i];
                    const auto found(std::find_if(dirs.begin(), dirs.end(), [&](const QString& d) {
                        return d != dir && dir.startsWith(d);
                    }));
                    if (found != dirs.end()) {
                        dirs.removeAt(i);
                        --max;
                        --i;
                    }
                }

                return std::move(dirs);
            };

            const QStringList libraryDirectories(prepareDirs(Settings::instance()->libraryDirectories()));

            const QStringList blacklistedDirectories(prepareDirs(Settings::instance()->blacklistedDirectories()));
            const auto isBlacklisted = [&blacklistedDirectories](const QString& path) {
                for (const QString& directory : blacklistedDirectories) {
                    if (path.startsWith(directory)) {
                        return true;
                    }
                }
                return false;
            };

            struct FileInDb
            {
                int id;
                bool embeddedMediaArtDeleted;
                long long modificationTime;
            };

            std::unordered_map<QString, FileInDb> filesInDb;
            std::unordered_map<QString, QString> mediaArtDirectoriesInDbHash;
            std::vector<int> filesToRemove;

            std::unordered_map<QString, bool> noMediaDirectories;
            const auto noMediaDirectoriesEnd(noMediaDirectories.end());
            const auto isNoMediaDirectory = [&noMediaDirectories, &noMediaDirectoriesEnd](const QString& directory) {
                {
                    const auto found(noMediaDirectories.find(directory));
                    if (found != noMediaDirectoriesEnd) {
                        return found->second;
                    }
                }
                const bool noMedia = QFileInfo(directory + QStringLiteral("/.nomedia")).isFile();
                noMediaDirectories.insert({directory, noMedia});
                return noMedia;
            };

            {
                // Extract tracks from database

                std::unordered_map<QString, bool> embeddedMediaArtExistanceHash;
                const auto embeddedMediaArtExistanceHashEnd(embeddedMediaArtExistanceHash.end());
                const auto checkExistance = [&](QString&& mediaArt) {
                    if (mediaArt.isEmpty()) {
                        return true;
                    }

                    const auto found(embeddedMediaArtExistanceHash.find(mediaArt));
                    if (found == embeddedMediaArtExistanceHashEnd) {
                        const QFileInfo fileInfo(mediaArt);
                        if (fileInfo.isFile() && fileInfo.isReadable()) {
                            embeddedMediaArtExistanceHash.insert({std::move(mediaArt), true});
                            return true;
                        }
                        embeddedMediaArtExistanceHash.insert({std::move(mediaArt), false});
                        return false;
                    }

                    return found->second;
                };

                QSqlQuery query(QLatin1String("SELECT id, filePath, modificationTime, directoryMediaArt, embeddedMediaArt FROM tracks ORDER BY id"), db);
                if (query.lastError().type() != QSqlError::NoError) {
                    qWarning() << "failed to get files from database" << query.lastError();
                    return;
                }

                while (query.next()) {
                    if (mCancel) {
                        return;
                    }

                    const int id(query.value(0).toInt());
                    QString filePath(query.value(1).toString());
                    const QFileInfo fileInfo(filePath);
                    QString directory(fileInfo.path());

                    bool remove = false;
                    if (!fileInfo.exists() || fileInfo.isDir() || !fileInfo.isReadable()) {
                        remove = true;
                    } else {
                        remove = true;
                        for (const QString& dir : libraryDirectories) {
                            if (filePath.startsWith(dir)) {
                                remove = false;
                                break;
                            }
                        }
                        if (!remove) {
                            remove = isBlacklisted(filePath);
                        }
                        if (!remove) {
                            remove = isNoMediaDirectory(directory);
                        }
                    }

                    if (remove) {
                        filesToRemove.push_back(id);
                    } else {
                        filesInDb.insert({std::move(filePath), FileInDb{id,
                                                                        !checkExistance(query.value(4).toString()),
                                                                        query.value(2).toLongLong()}});
                        mediaArtDirectoriesInDbHash.insert({std::move(directory), query.value(3).toString()});
                    }
                }
            }
            const auto filesInDbEnd(filesInDb.end());
            const auto mediaArtDirectoriesInDbHashEnd(mediaArtDirectoriesInDbHash.end());

            qInfo("Files in database: %zd (took %.3f s)", filesInDb.size(), static_cast<double>(stageTimer.restart()) / 1000.0);
            qInfo("Files to remove: %zd", filesToRemove.size());

            std::unordered_map<QString, QString> mediaArtDirectoriesHash;

            QString directory;
            QString directoryMediaArt;

            int foundFiles = 0;

            qInfo("Start scanning filesystem");
            emit mNotifier.stageChanged(LibraryUpdateRunnableNotifier::ScanningStage);

            for (const QString& topLevelDirectory : libraryDirectories) {
                if (mCancel) {
                    return;
                }

                QDirIterator iterator(topLevelDirectory, QDir::Files | QDir::Readable, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
                while (iterator.hasNext()) {
                    if (mCancel) {
                        return;
                    }

                    QString filePath(iterator.next());
                    const QFileInfo fileInfo(iterator.fileInfo());

                    const Extension extension = LibraryUtils::extensionFromSuffix(fileInfo.suffix());
                    if (extension == Extension::Other) {
                        continue;
                    }

                    if (fileInfo.path() != directory) {
                        directory = fileInfo.path();
                        directoryMediaArt = LibraryUtils::findMediaArtForDirectory(mediaArtDirectoriesHash, directory, mCancel);

                        const auto directoryMediaArtInDb(mediaArtDirectoriesInDbHash.find(directory));
                        if (directoryMediaArtInDb != mediaArtDirectoriesInDbHashEnd) {
                            if (directoryMediaArtInDb->second != directoryMediaArt) {
                                QSqlQuery query(db);
                                query.prepare(QStringLiteral("UPDATE tracks SET directoryMediaArt = ? WHERE instr(filePath, ?) = 1"));
                                query.addBindValue(nullIfEmpty(directoryMediaArt));
                                query.addBindValue(QString(directory % QLatin1Char('/')));
                                if (!query.exec()) {
                                    qWarning() << query.lastError();
                                }
                            }
                            mediaArtDirectoriesInDbHash.erase(directoryMediaArtInDb);
                        }
                    }

                    const auto foundInDb(filesInDb.find(filePath));
                    if (foundInDb == filesInDbEnd) {
                        // File is not in database

                        if (isNoMediaDirectory(fileInfo.path())) {
                            continue;
                        }

                        if (isBlacklisted(filePath)) {
                            continue;
                        }

                        filesToAdd.push_back({std::move(filePath), directoryMediaArt, extension});
                        ++foundFiles;
                        emit mNotifier.foundFilesChanged(foundFiles);
                    } else {
                        // File is in database

                        const FileInDb& file = foundInDb->second;

                        const long long modificationTime = getLastModifiedTime(filePath);
                        if (modificationTime == file.modificationTime) {
                            // File has not changed
                            if (file.embeddedMediaArtDeleted) {
                                const QString embeddedMediaArt(LibraryUtils::instance()->saveEmbeddedMediaArt(tagutils::getTrackInfo(filePath, extension, mimeDb).mediaArtData,
                                                                                                              embeddedMediaArtFiles,
                                                                                                              mimeDb));
                                QSqlQuery query(db);
                                query.prepare(QStringLiteral("UPDATE tracks SET embeddedMediaArt = ? WHERE id = ?"));
                                query.addBindValue(nullIfEmpty(embeddedMediaArt));
                                query.addBindValue(file.id);
                                if (!query.exec()) {
                                    qWarning() << query.lastError();
                                }
                            }
                        } else {
                            // File has changed
                            filesToRemove.push_back(file.id);
                            filesToAdd.push_back({foundInDb->first, directoryMediaArt, extension});
                            ++foundFiles;
                            emit mNotifier.foundFilesChanged(foundFiles);
                        }
                    }
                }
            }

            qInfo("End scanning filesystem (took %.3f s), need to extract tags from %d files", static_cast<double>(stageTimer.restart()) / 1000.0, foundFiles);

            if (!filesToRemove.empty()) {
                batchedCount(filesToRemove.size(), LibraryUtils::maxDbVariableCount, [&](size_t first, size_t count) {
                    if (mCancel) {
                        return;
                    }

                    QString queryString(QLatin1String("DELETE FROM tracks WHERE id IN ("));
                    queryString.push_back(QString::number(filesToRemove[first]));
                    for (std::size_t j = first + 1, max = j + count; j < max; ++j) {
                        queryString.push_back(QLatin1Char(','));
                        queryString.push_back(QString::number(filesToRemove[j]));
                    }
                    queryString.push_back(QLatin1Char(')'));
                    const QSqlQuery query(queryString, db);
                    if (query.lastError().type() != QSqlError::NoError) {
                        qWarning() << "Failed to remove files from database" << query.lastError();
                    }
                });

                qInfo("Removed %zd tracks from database (took %.3f s)", filesToRemove.size(), static_cast<double>(stageTimer.restart()) / 1000.0);
            }
        }

        if (mCancel) {
            return;
        }

        if (!filesToAdd.empty()) {
            qInfo("Start extracting tags from files");
            emit mNotifier.stageChanged(LibraryUpdateRunnableNotifier::ExtractingStage);

            LibraryTracksAdder adder(db);

            int count = 0;
            for (FileToAdd& file : filesToAdd) {
                if (mCancel) {
                    return;
                }

                tagutils::Info trackInfo(tagutils::getTrackInfo(file.filePath, file.extension, mimeDb));
                if (trackInfo.fileTypeValid) {
                    ++count;

                    if (trackInfo.title.isEmpty()) {
                        trackInfo.title = QFileInfo(file.filePath).fileName();
                    }

                    adder.addTrackToDatabase(file.filePath,
                                             getLastModifiedTime(file.filePath),
                                             trackInfo,
                                             file.directoryMediaArt,
                                             LibraryUtils::instance()->saveEmbeddedMediaArt(trackInfo.mediaArtData,
                                                                                            embeddedMediaArtFiles,
                                                                                            mimeDb));
                    emit mNotifier.extractedFilesChanged(count);
                }
            }
            qInfo("Added %d tracks to database (took %.3f s)", count, static_cast<double>(stageTimer.restart()) / 1000.0);
        }

        if (mCancel) {
            return;
        }

        emit mNotifier.stageChanged(LibraryUpdateRunnableNotifier::FinishingStage);

        std::unordered_set<QString> allEmbeddedMediaArt;
        {
            QSqlQuery query(QLatin1String("SELECT DISTINCT(embeddedMediaArt) FROM tracks WHERE embeddedMediaArt != ''"), db);
            if (query.lastError().type() == QSqlError::NoError) {
                if (query.last()) {
                    if (query.at() > 0) {
                        allEmbeddedMediaArt.reserve(static_cast<size_t>(query.at() + 1));
                    }
                    query.seek(QSql::BeforeFirstRow);
                    while (query.next()) {
                        if (mCancel) {
                            return;
                        }

                        allEmbeddedMediaArt.insert(query.value(0).toString());
                    }
                }
            }
        }

        {
            const QFileInfoList files(QDir(mMediaArtDirectory).entryInfoList(QDir::Files));
            for (const QFileInfo& info : files) {
                if (mCancel) {
                    return;
                }

                if (!contains(allEmbeddedMediaArt, info.filePath())) {
                    if (!QFile::remove(info.filePath())) {
                        qWarning() << "failed to remove file:" << info.filePath();
                    }
                }
            }
        }

        {
            QSqlQuery query(db);

            if (!query.exec(QLatin1String("DELETE FROM artists "
                                          "WHERE id IN ("
                                            "SELECT id FROM artists "
                                            "LEFT JOIN tracks_artists ON tracks_artists.artistId = artists.id "
                                            "WHERE tracks_artists.artistId IS NULL"
                                          ")"))) {
                qWarning() << query.lastError();
            } else {
                if (query.exec(QLatin1String("SELECT changes()"))) {
                    query.next();
                    qInfo("Removed %d artists", query.value(0).toInt());
                }
            }
            if (!query.exec(QLatin1String("DELETE FROM albums "
                                          "WHERE id IN ("
                                            "SELECT id FROM albums "
                                            "LEFT JOIN tracks_albums ON tracks_albums.albumId = albums.id "
                                            "WHERE tracks_albums.albumId IS NULL"
                                          ")"))) {
                qWarning() << query.lastError();
            } else {
                if (query.exec(QLatin1String("SELECT changes()"))) {
                    query.next();
                    qInfo("Removed %d albums", query.value(0).toInt());
                }
            }
            if (!query.exec(QLatin1String("DELETE FROM genres "
                                          "WHERE id IN ("
                                            "SELECT id FROM genres "
                                            "LEFT JOIN tracks_genres ON tracks_genres.genreId = genres.id "
                                            "WHERE tracks_genres.genreId IS NULL"
                                          ")"))) {
                qWarning() << query.lastError();
            } else {
                if (query.exec(QLatin1String("SELECT changes()"))) {
                    query.next();
                    qInfo("Removed %d genres", query.value(0).toInt());
                }
            }
        }

        qInfo("End updating database (last stage took %.3f s)", static_cast<double>(stageTimer.elapsed()) / 1000.0);
        qInfo("Total time: %.3f s", static_cast<double>(timer.elapsed()) / 1000.0);
    }


}
