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

#include "libraryutils.h"

#include <functional>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QThreadPool>
#include <QtConcurrentRun>

#include "albumsmodel.h"
#include "mediaartutils.h"
#include "settings.h"
#include "sqlutils.h"
#include "stdutils.h"
#include "tagutils.h"
#include "utilsfunctions.h"

namespace unplayer
{
    namespace
    {
        const QLatin1String removeFilesConnectionName("unplayer_remove");
        const QLatin1String saveTagsConnectionName("unplayer_save");

        inline QString emptyIfNull(const QString& string)
        {
            if (string.isNull()) {
                return QLatin1String("");
            }
            return string;
        }

        const int databaseVersion = 1;

        const QString& databasePath()
        {
            static const QString path(QString::fromLatin1("%1/library.sqlite").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation)));
            return path;
        }

        void deleteFilesFromFilesystem(const std::vector<QString>& paths)
        {
            for (const QString& path : paths) {
                if (!QFile::remove(path)) {
                    qWarning() << "Failed to remove file:" << path;
                }
            }
        }

        void deleteDirectoriesFromFilesystem(const std::vector<QString>& paths)
        {
            for (const QString& path : paths) {
                QDir dir(path);
                if (dir.exists()) {
                    if (!dir.removeRecursively()) {
                        qWarning() << "Failed to remove directory:" << path;
                    }
                } else {
                    qWarning() << "Path is not directory:" << path;
                }
            }
        }

        bool removeTracksFromDbByPaths(const std::vector<QString>& filePaths, const QSqlDatabase& db)
        {
            if (!filePaths.empty()) {
                bool abort = false;

                batchedCount(filePaths.size(), LibraryUtils::maxDbVariableCount, [&](size_t first, size_t count) {
                    if (abort) {
                        return;
                    }
                    if (!qApp) {
                        abort = true;
                        return;
                    }

                    QString queryString(QLatin1String("DELETE FROM tracks WHERE filePath IN (?"));
                    const QLatin1String add(",?");
                    queryString.reserve(queryString.size() + (static_cast<int>(count) - 1) * add.size() + 1);
                    for (size_t i = 1; i < count; ++i) {
                        queryString += add;
                    }
                    queryString += QLatin1Char(')');

                    QSqlQuery query(db);
                    if (query.prepare(queryString)) {
                        for (size_t i = first, max = first + count; i < max; ++i) {
                            query.addBindValue(filePaths[i]);
                        }
                        if (!query.exec()) {
                            qWarning() << "Failed to remove tracks from database" << query.lastError();
                            abort = true;
                        }
                    } else {
                        qWarning() << "Failed to remove tracks from database" << query.lastError();
                        abort = true;
                    }
                });

                return !abort;
            }

            return false;
        }

        bool removeTracksFromDbByDirectories(const std::vector<QString>& paths, const QSqlDatabase& db)
        {
            if (!paths.empty()) {
                const auto addTrailingSeparator = [](const QString& path) -> QString {
                    return path.endsWith(QLatin1Char('/')) ? path : path % QLatin1Char('/');
                };

                bool abort = false;

                batchedCount(paths.size(), LibraryUtils::maxDbVariableCount, [&](size_t first, size_t count) {
                    if (abort) {
                        return;
                    }
                    if (!qApp) {
                        abort = true;
                        return;
                    }

                    QString queryString(QLatin1String("DELETE FROM tracks WHERE instr(filePath, ?) = 1"));
                    const QLatin1String add(" OR instr(filePath, ?) = 1");
                    queryString.reserve(queryString.size() + (static_cast<int>(count) - 1) * add.size() + 1);
                    for (size_t i = 1; i < count; ++i) {
                        queryString += add;
                    }
                    queryString += QLatin1Char(')');

                    QSqlQuery query(db);
                    if (query.prepare(queryString)) {
                        for (size_t i = first, max = first + count; i < max; ++i) {
                            query.addBindValue(addTrailingSeparator(paths[i]));
                        }
                        if (!query.exec()) {
                            qWarning() << "Failed to remove tracks from database" << query.lastError();
                            abort = true;
                        }
                    } else {
                        qWarning() << "Failed to remove tracks from database" << query.lastError();
                        abort = true;
                    }
                });

                return !abort;
            }

            return false;
        }

        QString makeInStringFromIds(const std::vector<int>& ids, size_t first, size_t count, bool& addNull)
        {
            QString str(QString::number(ids[first]));
            for (size_t i = first + 1, max = first + count; i <= max; ++i) {
                const int id = ids[i];
                if (id > 0) {
                    str += QLatin1Char(',');
                    str += QString::number(id);
                } else {
                    addNull = true;
                }
            }
            str += QLatin1Char(')');
            return str;
        }

        QString makeInStringFromIds(const std::vector<int>& ids, size_t first, size_t count)
        {
            bool addNull;
            return makeInStringFromIds(ids, first, count, addNull);
        }
    }

    const QString LibraryUtils::databaseType(QLatin1String("QSQLITE"));
    const size_t LibraryUtils::maxDbVariableCount = 999; // SQLITE_MAX_VARIABLE_NUMBER

    QSqlDatabase LibraryUtils::openDatabase(const QString& connectionName)
    {
        auto db(QSqlDatabase::addDatabase(databaseType, connectionName));
        db.setDatabaseName(databasePath());
        if (!db.open()) {
            qWarning() << "Failed to open database:" << db.lastError();
        }
        QSqlQuery query(db);
        if (!query.exec(QLatin1String("PRAGMA foreign_keys = ON"))) {
            qWarning() << "Failed to enable foreign keys:" << query.lastError();
        }
        return db;
    }

    LibraryUtils* LibraryUtils::instance()
    {
        static auto const p = new LibraryUtils(qApp);
        return p;
    }

    bool LibraryUtils::removeTracksFromDbByIds(const std::vector<int>& ids, const QSqlDatabase& db, const std::atomic_bool& cancel)
    {
        if (!ids.empty()) {
            bool abort = false;
            batchedCount(ids.size(), maxDbVariableCount, [&](size_t first, size_t count) {
                if (abort) {
                    return;
                }
                if (cancel) {
                    abort = true;
                    return;
                }
                if (!qApp) {
                    abort = true;
                    return;
                }
                QSqlQuery query(db);
                if (!query.exec(QLatin1String("DELETE FROM tracks WHERE id IN (") % makeInStringFromIds(ids, first, count))) {
                    qWarning() << "Failed to remove tracks from database" << query.lastError();
                    abort = true;
                }
            });
            return !abort;
        }

        return false;
    }

    void LibraryUtils::removeUnusedCategories(const QSqlDatabase& db)
    {
        QSqlQuery query(db);

        const auto removed = [&] {
            if (query.exec(QLatin1String("SELECT changes()"))) {
                query.next();
                return query.value(0).toInt();
            }
            return -1;
        };

        if (query.exec(QLatin1String("DELETE FROM albums "
                                     "WHERE id IN ("
                                       "SELECT id FROM albums "
                                       "LEFT JOIN tracks_albums ON albumId = id "
                                       "WHERE trackId IS NULL"
                                     ")"))) {
            const int count = removed();
            if (count != 0) {
                qInfo("Removed %d albums", count);
            }
        } else {
            qWarning() << query.lastError();
        }
        if (query.exec(QLatin1String("DELETE FROM artists "
                                     "WHERE id IN ("
                                       "SELECT id FROM artists "
                                       "LEFT JOIN tracks_artists ON artistId = id "
                                       "WHERE trackId IS NULL"
                                     ")"))) {
            const int count = removed();
            if (count != 0) {
                qInfo("Removed %d artists", count);
            }
        } else {
            qWarning() << query.lastError();
        }
        if (query.exec(QLatin1String("DELETE FROM genres "
                                     "WHERE id IN ("
                                       "SELECT id FROM genres "
                                       "LEFT JOIN tracks_genres ON genreId = id "
                                       "WHERE trackId IS NULL"
                                     ")"))) {
            const int count = removed();
            if (count != 0) {
                qInfo("Removed %d genres", count);
            }
        } else {
            qWarning() << query.lastError();
        }
    }

    void LibraryUtils::removeUnusedMediaArt(const QSqlDatabase& db, const std::atomic_bool& cancel)
    {
        QSqlQuery query(db);

        std::unordered_set<QString> allEmbeddedMediaArt;
        if (query.exec(QLatin1String("SELECT DISTINCT embeddedMediaArt FROM tracks WHERE embeddedMediaArt IS NOT NULL"))) {
            if (reserveFromQuery(allEmbeddedMediaArt, query) > 0) {
                while (query.next()) {
                    allEmbeddedMediaArt.insert(query.value(0).toString());
                }
            }
        } else {
            qWarning() << query.lastError();
            return;
        }

        if (cancel) {
            return;
        }

        std::unordered_set<QString> allUserMediaArt;
        if (query.exec(QLatin1String("SELECT DISTINCT userMediaArt FROM albums WHERE userMediaArt IS NOT NULL"))) {
            if (reserveFromQuery(allUserMediaArt, query) > 0) {
                while (query.next()) {
                    allUserMediaArt.insert(query.value(0).toString());
                }
            }
        } else {
            qWarning() << query.lastError();
            return;
        }

        if (cancel) {
            return;
        }

        const QFileInfoList files(QDir(MediaArtUtils::mediaArtDirectory()).entryInfoList(QDir::Files));
        for (const QFileInfo& info : files) {
            if (cancel) {
                return;
            }
            const QString filePath(info.filePath());
            if (!contains(allEmbeddedMediaArt, filePath) && !contains(allUserMediaArt, filePath)) {
                if (!QFile::remove(filePath)) {
                    qWarning() << "Failed to remove file:" << info.filePath();
                }
            }
        }
    }

    void LibraryUtils::initDatabase()
    {
        qDebug() << "Initializing database";

        const QString dataDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
        if (!QDir().mkpath(dataDir)) {
            qWarning() << "failed to create data directory";
            return;
        }

        QSqlDatabase db(openDatabase());
        if (!db.isOpen()) {
            return;
        }

        QSqlQuery query(db);

        if (db.tables().isEmpty()) {
            qInfo("Creating tables");

            const auto exec = [&](const char* string) {
                if (!query.exec(QLatin1String(string))) {
                    qWarning() << "initDatabase" << query.lastError();
                    return false;
                }
                return true;
            };

            if (!exec("CREATE TABLE tracks ("
                      "    id INTEGER PRIMARY KEY,"
                      "    filePath TEXT NOT NULL,"
                      "    modificationTime INTEGER NOT NULL,"
                      "    title TEXT COLLATE NOCASE,"
                      "    year INTEGER,"
                      "    trackNumber INTEGER,"
                      "    discNumber TEXT,"
                      "    duration INTEGER NOT NULL,"
                      "    directoryMediaArt TEXT,"
                      "    embeddedMediaArt TEXT"
                      ")")) {
                return;
            }

            if (!exec("CREATE TABLE artists ("
                      "    id INTEGER PRIMARY KEY,"
                      "    title TEXT NOT NULL COLLATE NOCASE"
                      ")")) {
                return;
            }

            if (!exec("CREATE TABLE albums ("
                      "    id INTEGER PRIMARY KEY,"
                      "    title TEXT NOT NULL COLLATE NOCASE,"
                      "    userMediaArt TEXT"
                      ")")) {
                return;
            }

            if (!exec("CREATE TABLE genres ("
                      "    id INTEGER PRIMARY KEY,"
                      "    title TEXT NOT NULL COLLATE NOCASE"
                      ")")) {
                return;
            }

            if (!exec("CREATE TABLE tracks_artists ("
                      "    trackId INTEGER NOT NULL REFERENCES tracks(id) ON DELETE CASCADE,"
                      "    artistId INTEGER NOT NULL REFERENCES artists(id)"
                      ")")) {
                return;
            }

            if (!exec("CREATE INDEX tracks_artists_trackIndex ON tracks_artists(trackId)")) {
                return;
            }

            if (!exec("CREATE TABLE tracks_albums ("
                      "    trackId INTEGER NOT NULL REFERENCES tracks(id) ON DELETE CASCADE,"
                      "    albumId INTEGER NOT NULL REFERENCES albums(id)"
                      ")")) {
                return;
            }

            if (!exec("CREATE INDEX tracks_albums_trackIndex ON tracks_albums(trackId)")) {
                return;
            }

            if (!exec("CREATE TABLE albums_artists ("
                      "    albumId INTEGER NOT NULL REFERENCES albums(id) ON DELETE CASCADE,"
                      "    artistId INTEGER NOT NULL REFERENCES artists(id)"
                      ")")) {
                return;
            }

            if (!exec("CREATE INDEX albums_artists_albumIndex ON albums_artists(albumId)")) {
                return;
            }

            if (!exec("CREATE TABLE tracks_genres ("
                      "    trackId INTEGER NOT NULL REFERENCES tracks(id) ON DELETE CASCADE,"
                      "    genreId INTEGER NOT NULL REFERENCES genres(id)"
                      ")")) {
                return;
            }

            if (!exec("CREATE INDEX tracks_genres_trackIndex ON tracks_genres(trackId)")) {
                return;
            }

            query.exec(QString::fromLatin1("PRAGMA user_version = %1").arg(databaseVersion));

            mCreatedTables = true;
        } else {
            query.exec(QLatin1String("PRAGMA user_version"));
            query.next();
            const int currentVersion = query.value(0).toInt();
            if (currentVersion != databaseVersion) {
                // TODO: migration
                return;
            }
        }

        qInfo() << "Database initialized, file path:" << databasePath();

        mDatabaseInitialized = true;
    }

    void LibraryUtils::updateDatabase()
    {
        if (mLibraryUpdateRunnable || !mDatabaseInitialized) {
            return;
        }

        auto runnable = new LibraryUpdateRunnable();
        QObject::connect(runnable, &LibraryUpdateRunnable::stageChanged, this, [this](UpdateStage newStage) {
            mLibraryUpdateStage = newStage;
            emit updateStageChanged();
        });
        QObject::connect(runnable, &LibraryUpdateRunnable::foundFilesChanged, this, [this](int found) {
            mFoundTracks = found;
            emit foundTracksChanged();
        });
        QObject::connect(runnable, &LibraryUpdateRunnable::extractedFilesChanged, this, [this](int extracted) {
            mExtractedTracks = extracted;
            emit extractedTracksChanged();
        });
        QObject::connect(runnable, &LibraryUpdateRunnable::finished, this, [this]() {
            mLibraryUpdateRunnable = nullptr;
            mLibraryUpdateStage = LibraryUpdateRunnable::NoneStage;
            mFoundTracks = 0;
            mExtractedTracks = 0;
            emit updatingChanged();
            emit updateStageChanged();
            emit foundTracksChanged();
            emit extractedTracksChanged();
            emit databaseChanged();
        });
        QObject::connect(qApp, &QCoreApplication::aboutToQuit, this, [this] {
            if (mLibraryUpdateRunnable) {
                static_cast<LibraryUpdateRunnable*>(mLibraryUpdateRunnable)->cancel();
            }
        });
        mLibraryUpdateRunnable = runnable;
        mLibraryUpdateStage = LibraryUpdateRunnable::PreparingStage;
        QThreadPool::globalInstance()->start(runnable);
        emit updatingChanged();
        emit updateStageChanged();
    }

    void LibraryUtils::cancelDatabaseUpdate()
    {
        if (mLibraryUpdateRunnable) {
            static_cast<LibraryUpdateRunnable*>(mLibraryUpdateRunnable)->cancel();
        }
    }

    void LibraryUtils::resetDatabase()
    {
        if (!mDatabaseInitialized) {
            return;
        }

        qInfo("Resetting database");

        MediaArtUtils::deleteInstance();

        const QStringList connections(QSqlDatabase::connectionNames());
        if (!connections.isEmpty()) {
            if (connections.size() > 1 || connections.first() != QSqlDatabase::defaultConnection) {
                qWarning() << "There should be only default connection in resetDatabase(), connections:" << connections;
                return;
            }
            QSqlDatabase::database(QSqlDatabase::defaultConnection, false).close();
            QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
        }

        if (!QFile::remove(databasePath())) {
            qWarning() << "Failed to remove database file:" << databasePath();
        }
        initDatabase();

        if (!QDir(MediaArtUtils::mediaArtDirectory()).removeRecursively()) {
            qWarning() << "Failed to remove media art directory:" << MediaArtUtils::mediaArtDirectory();
        }

        emit databaseChanged();
    }

    bool LibraryUtils::isDatabaseInitialized() const
    {
        return mDatabaseInitialized;
    }

    bool LibraryUtils::isCreatedTables() const
    {
        return mCreatedTables;
    }

    int LibraryUtils::artistsCount() const
    {
        if (mDatabaseInitialized) {
            QSqlQuery query;
            if (query.exec(QLatin1String("SELECT COUNT(*) FROM artists"))) {
                if (query.next()) {
                    return query.value(0).toInt();
                }
            } else {
                qWarning() << query.lastError();
            }
        }
        return 0;
    }

    int LibraryUtils::albumsCount() const
    {
        if (mDatabaseInitialized) {
            QSqlQuery query;
            if (query.exec(QLatin1String("SELECT COUNT(*) FROM albums"))) {
                if (query.next()) {
                    return query.value(0).toInt();
                }
            } else {
                qWarning() << query.lastError();
            }
        }
        return 0;
    }

    int LibraryUtils::tracksCount() const
    {
        if (mDatabaseInitialized) {
            QSqlQuery query;
            if (query.exec(QLatin1String("SELECT COUNT(*) FROM tracks"))) {
                if (query.next()) {
                    return query.value(0).toInt();
                }
            } else {
                qWarning() << query.lastError();
            }
        }
        return 0;
    }

    int LibraryUtils::tracksDuration() const
    {
        if (mDatabaseInitialized) {
            QSqlQuery query;
            if (query.exec(QLatin1String("SELECT SUM(duration) FROM tracks"))) {
                if (query.next()) {
                    return query.value(0).toInt();
                }
            } else {
                qWarning() << query.lastError();
            }
        }
        return 0;
    }

    bool LibraryUtils::isUpdating() const
    {
        return mLibraryUpdateRunnable;
    }

    LibraryUtils::UpdateStage LibraryUtils::updateStage() const
    {
        return mLibraryUpdateStage;
    }

    int LibraryUtils::foundTracks() const
    {
        return mFoundTracks;
    }

    int LibraryUtils::extractedTracks() const
    {
        return mExtractedTracks;
    }

    bool LibraryUtils::isRemovingFiles() const
    {
        return mRemovingFiles;
    }

    void LibraryUtils::removeArtists(std::vector<int>&& artists, bool deleteFiles)
    {
        if (mRemovingFiles || !mDatabaseInitialized || artists.empty()) {
            return;
        }

        mRemovingFiles = true;
        emit removingFilesChanged();

        auto future = QtConcurrent::run([deleteFiles, artists = std::move(artists)] {
            qInfo("Start removing artists, count=%zu, deleteFiles=%d", artists.size(), deleteFiles);

            QElapsedTimer timer;
            timer.start();

            // Open database
            const DatabaseConnectionGuard databaseGuard{removeFilesConnectionName};
            QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
            if (!db.isOpen()) {
                return;
            }

            const TransactionGuard transactionGuard(db);

            QSqlQuery query;
            std::vector<QString> paths;

            bool abort = false;
            batchedCount(artists.size(), maxDbVariableCount, [&](size_t first, size_t count) {
                if (abort) {
                    return;
                }
                if (!qApp) {
                    abort = true;
                    return;
                }

                bool addNull = false;
                QString whereString(QLatin1String("WHERE artistId IN (") % makeInStringFromIds(artists, first, count, addNull));
                if (addNull) {
                    whereString += QLatin1String(" OR artistId IS NULL");
                }

                if (deleteFiles) {
                    if (query.exec(QLatin1String("SELECT filePath "
                                                 "FROM tracks "
                                                 "LEFT JOIN tracks_artists ON tracks_artists.trackId = id ") % whereString)) {
                        if (reserveFromQueryAppend(paths, query) > 0) {
                            while (query.next()) {
                                paths.push_back(query.value(0).toString());
                            }
                        }
                    } else {
                        qWarning() << "Failed to get tracks for artists" << query.lastError();
                        abort = true;
                        return;
                    }
                }

                if (!query.exec(QLatin1String("DELETE FROM tracks "
                                              "WHERE id IN ("
                                                "SELECT id FROM tracks "
                                                "LEFT JOIN tracks_artists ON tracks_artists.trackId = id ") %
                                                whereString %
                                              QLatin1Char(')'))) {
                    qWarning() << "Failed to remove tracks from database" << query.lastError();
                    abort = true;
                }
            });

            if (!abort) {
                if (deleteFiles) {
                    deleteFilesFromFilesystem(paths);
                }

                removeUnusedCategories(db);
                removeUnusedMediaArt(db);

                qInfo("Artists removing time: %lld ms", static_cast<long long>(timer.elapsed()));
            }
        });

        onFutureFinished(future, this, [this] {
            mRemovingFiles = false;
            emit removingFilesChanged();
            emit databaseChanged();
        });
    }

    void LibraryUtils::removeAlbums(std::vector<int>&& albums, bool deleteFiles)
    {
        if (mRemovingFiles || !mDatabaseInitialized || albums.empty()) {
            return;
        }

        mRemovingFiles = true;
        emit removingFilesChanged();

        auto future = QtConcurrent::run([deleteFiles, albums = std::move(albums)] {
            qInfo("Start removing albums, count=%zu, deleteFiles=%d", albums.size(), deleteFiles);

            QElapsedTimer timer;
            timer.start();

            // Open database
            const DatabaseConnectionGuard databaseGuard{removeFilesConnectionName};
            QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
            if (!db.isOpen()) {
                return;
            }

            const TransactionGuard transactionGuard(db);

            QSqlQuery query;
            std::vector<QString> paths;

            bool abort = false;
            batchedCount(albums.size(), maxDbVariableCount, [&](size_t first, size_t count) {
                if (abort) {
                    return;
                }
                if (!qApp) {
                    abort = true;
                    return;
                }

                bool addNull = false;
                QString whereString(QLatin1String("WHERE albumId IN (") % makeInStringFromIds(albums, first, count, addNull));
                if (addNull) {
                    whereString += QLatin1String(" OR albumId IS NULL");
                }

                if (deleteFiles) {
                    if (query.exec(QLatin1String("SELECT filePath "
                                                 "FROM tracks "
                                                 "LEFT JOIN tracks_albums ON tracks_albums.trackId = id ") % whereString)) {
                        if (reserveFromQueryAppend(paths, query) > 0) {
                            while (query.next()) {
                                paths.push_back(query.value(0).toString());
                            }
                        }
                    } else {
                        qWarning() << "Failed to get tracks for albums" << query.lastError();
                        abort = true;
                        return;
                    }
                }

                if (!query.exec(QLatin1String("DELETE FROM tracks "
                                              "WHERE id IN ("
                                                "SELECT id FROM tracks "
                                                "LEFT JOIN tracks_albums ON tracks_albums.trackId = id ") %
                                                whereString %
                                              QLatin1Char(')'))) {
                    qWarning() << "Failed to remove tracks from database" << query.lastError();
                    abort = true;
                }
            });

            if (!abort) {
                if (deleteFiles) {
                    deleteFilesFromFilesystem(paths);
                }

                removeUnusedCategories(db);
                removeUnusedMediaArt(db);

                qInfo("Albums removing time: %lld ms", static_cast<long long>(timer.elapsed()));
            }
        });

        onFutureFinished(future, this, [this] {
            mRemovingFiles = false;
            emit removingFilesChanged();
            emit databaseChanged();
        });
    }

    void LibraryUtils::removeGenres(std::vector<int>&& genres, bool deleteFiles)
    {
        if (mRemovingFiles || !mDatabaseInitialized || genres.empty()) {
            return;
        }

        mRemovingFiles = true;
        emit removingFilesChanged();

        auto future = QtConcurrent::run([deleteFiles, genres = std::move(genres)] {
            qInfo("Start removing genres, count=%zu, deleteFiles=%d", genres.size(), deleteFiles);

            QElapsedTimer timer;
            timer.start();

            // Open database
            const DatabaseConnectionGuard databaseGuard{removeFilesConnectionName};
            QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
            if (!db.isOpen()) {
                return;
            }

            const TransactionGuard transactionGuard(db);

            QSqlQuery query;
            std::vector<QString> paths;

            bool abort = false;
            batchedCount(genres.size(), maxDbVariableCount, [&](size_t first, size_t count) {
                if (abort) {
                    return;
                }
                if (!qApp) {
                    abort = true;
                    return;
                }

                const QString whereString(QLatin1String("WHERE genreId IN (") % makeInStringFromIds(genres, first, count));

                if (deleteFiles) {
                    if (query.exec(QLatin1String("SELECT filePath "
                                                 "FROM tracks "
                                                 "JOIN tracks_genres ON tracks_genres.trackId = id ") % whereString)) {
                        if (reserveFromQueryAppend(paths, query) > 0) {
                            while (query.next()) {
                                paths.push_back(query.value(0).toString());
                            }
                        }
                    } else {
                        qWarning() << "Failed to get tracks for genres" << query.lastError();
                        abort = true;
                        return;
                    }
                }

                if (!query.exec(QLatin1String("DELETE FROM tracks "
                                              "WHERE id IN ("
                                                "SELECT id FROM tracks "
                                                "JOIN tracks_genres ON tracks_genres.trackId = id ") %
                                                whereString %
                                              QLatin1Char(')'))) {
                    qWarning() << "Failed to remove tracks from database" << query.lastError();
                    abort = true;
                }
            });

            if (!abort) {
                if (deleteFiles) {
                    deleteFilesFromFilesystem(paths);
                }

                removeUnusedCategories(db);
                removeUnusedMediaArt(db);

                qInfo("Genres removing time: %lld ms", static_cast<long long>(timer.elapsed()));
            }
        });

        onFutureFinished(future, this, [this] {
            mRemovingFiles = false;
            emit removingFilesChanged();
            emit databaseChanged();
        });
    }

    void LibraryUtils::removeTracks(std::vector<LibraryTrack>&& tracks, bool deleteFiles)
    {
        if (mRemovingFiles || !mDatabaseInitialized || tracks.empty()) {
            return;
        }

        mRemovingFiles = true;
        emit removingFilesChanged();

        auto future = QtConcurrent::run([deleteFiles, tracks = std::move(tracks)]() mutable {
            qInfo("Start removing tracks, count=%zu, deleteFiles=%d", tracks.size(), deleteFiles);

            QElapsedTimer timer;
            timer.start();

            // Open database
            const DatabaseConnectionGuard databaseGuard{removeFilesConnectionName};
            QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
            if (!db.isOpen()) {
                return;
            }

            const TransactionGuard transactionGuard(db);

            std::vector<int> ids;
            ids.reserve(tracks.size());
            if (deleteFiles) {
                std::vector<QString> paths;
                paths.reserve(tracks.size());
                for (auto& track : tracks) {
                    ids.push_back(track.id);
                    paths.push_back(std::move(track.filePath));
                }
                if (removeTracksFromDbByIds(std::move(ids), db)) {
                    deleteFilesFromFilesystem(paths);
                }
            } else {
                for (auto& track : tracks) {
                    ids.push_back(track.id);
                }
                removeTracksFromDbByIds(std::move(ids), db);
            }

            removeUnusedCategories(db);
            removeUnusedMediaArt(db);

            qInfo("Tracks removing time: %lld ms", static_cast<long long>(timer.elapsed()));
        });

        onFutureFinished(future, this, [this] {
            mRemovingFiles = false;
            emit removingFilesChanged();
            emit databaseChanged();
        });
    }

    void LibraryUtils::removeTracksByPaths(std::vector<QString>&& paths, bool deleteFiles, bool deleteDirectories)
    {
        if (mRemovingFiles || !mDatabaseInitialized || paths.empty()) {
            return;
        }

        mRemovingFiles = true;
        emit removingFilesChanged();

        auto future = QtConcurrent::run([deleteFiles, deleteDirectories, paths = std::move(paths)]() mutable {
            qInfo("Start removing files, count=%zu", paths.size());

            QElapsedTimer timer;
            timer.start();

            // Open database
            const DatabaseConnectionGuard databaseGuard{removeFilesConnectionName};
            QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
            if (!db.isOpen()) {
                return;
            }

            const TransactionGuard transactionGuard(db);

            if (deleteDirectories) {
                std::vector<QString> files;
                files.reserve(paths.size());
                std::vector<QString> directories;
                for (auto& path : paths) {
                    if (QFileInfo(path).isDir()) {
                        directories.push_back(std::move(path));
                    } else {
                        files.push_back(std::move(path));
                    }
                    //const bool isDir = QFileInfo(path).isDir();
                    //files.emplace_back(std::move(path), isDir);
                }
                if (removeTracksFromDbByPaths(files, db)) {
                    if (removeTracksFromDbByDirectories(directories, db)) {
                        if (deleteFiles) {
                            deleteFilesFromFilesystem(files);
                            deleteDirectoriesFromFilesystem(directories);
                        }
                    }
                }
            } else {
                if (removeTracksFromDbByPaths(paths, db)) {
                    if (deleteFiles) {
                        deleteFilesFromFilesystem(paths);
                    }
                }
            }

            removeUnusedCategories(db);
            removeUnusedMediaArt(db);

            qInfo("Files removing time: %lld ms", static_cast<long long>(timer.elapsed()));
        });

        onFutureFinished(future, this, [this] {
            mRemovingFiles = false;
            emit removingFilesChanged();
            emit databaseChanged();
        });
    }

    bool LibraryUtils::isSavingTags() const
    {
        return mSavingTags;
    }

    void LibraryUtils::saveTags(const QStringList& files, const QVariantMap& tags, bool incrementTrackNumber)
    {
        if (mSavingTags || !mDatabaseInitialized) {
            return;
        }

        mSavingTags = true;
        emit savingTagsChanged();

        auto future = QtConcurrent::run([files, tags, incrementTrackNumber]() {
            qInfo("Start saving tags");

            QElapsedTimer timer;
            timer.start();

            QMimeDatabase mimeDb;

            if (!QDir().mkpath(MediaArtUtils::mediaArtDirectory())) {
                qWarning() << "failed to create media art directory:" << MediaArtUtils::mediaArtDirectory();
            }
            std::unordered_map<QByteArray, QString> embeddedMediaArtFiles(MediaArtUtils::getEmbeddedMediaArtFiles());

            std::vector<QString> embeddedMediaArt;
            embeddedMediaArt.reserve(static_cast<size_t>(files.size()));
            const auto callback = [&](tagutils::Info& info) {
                QString path(MediaArtUtils::saveEmbeddedMediaArt(info.mediaArtData, embeddedMediaArtFiles, mimeDb));
                embeddedMediaArt.push_back(std::move(path));
                info.mediaArtData.clear();
            };

            std::vector<tagutils::Info> infos(incrementTrackNumber ? tagutils::saveTags<true>(files, tags, mimeDb, callback)
                                                                   : tagutils::saveTags<false>(files, tags, mimeDb, callback));
            if (!qApp) {
                return;
            }

            const DatabaseConnectionGuard databaseGuard{saveTagsConnectionName};

            // Open database
            QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
            if (!db.isOpen()) {
                return;
            }

            const TransactionGuard transactionGuard(db);

            batchedCount(infos.size(), LibraryUtils::maxDbVariableCount, [&](size_t first, size_t count) {
                if (!qApp) {
                    return;
                }

                QString queryString(QLatin1String("DELETE FROM tracks WHERE filePath = ?"));
                for (size_t i = first + 1, max = first + count; i < max; ++i) {
                    queryString.push_back(QLatin1String(" OR filePath = ?"));
                }

                QSqlQuery query(db);
                query.prepare(queryString);
                for (size_t i = first, max = first + count; i < max; ++i) {
                    query.addBindValue(emptyIfNull(infos[i].filePath));
                }

                if (!query.exec()) {
                    qWarning() << "failed to remove tracks:" << query.lastError();
                }
            });

            std::unordered_map<QString, QString> mediaArtDirectoriesHash;

            LibraryTracksAdder adder(db);

            for (size_t i = 0, max = infos.size(); i < max; ++i) {
                tagutils::Info& info = infos[i];
                QFileInfo fileInfo(info.filePath);
                if (info.title.isEmpty()) {
                    info.title = fileInfo.fileName();
                }
                const QString directoryMediaArt(MediaArtUtils::findMediaArtForDirectory(mediaArtDirectoriesHash, fileInfo.path(), false));
                adder.addTrackToDatabase(info.filePath, getLastModifiedTime(info.filePath), info, directoryMediaArt, embeddedMediaArt[i]);
            }

            qInfo("Done saving tags, %lldms", timer.elapsed());
        });

        onFutureFinished(future, this, [this] {
            mSavingTags = false;
            emit savingTagsChanged();
            emit databaseChanged();
        });
    }

    LibraryUtils::LibraryUtils(QObject* parent)
        : QObject(parent),
          mDatabaseInitialized(false),
          mCreatedTables(false),
          mLibraryUpdateRunnable(nullptr),
          mLibraryUpdateStage(LibraryUpdateRunnable::NoneStage),
          mFoundTracks(0),
          mExtractedTracks(0),
          mRemovingFiles(false),
          mSavingTags(false)
    {
        qRegisterMetaType<LibraryUpdateRunnable::UpdateStage>();
        initDatabase();
        QObject::connect(this, &LibraryUtils::databaseChanged, this, &LibraryUtils::mediaArtChanged);
    }
}
