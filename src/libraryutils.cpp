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
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QMimeDatabase>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QThreadPool>
#include <QUuid>
#include <QtConcurrentRun>

#include "albumsmodel.h"
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
    }

    QString mediaArtFromQuery(const QSqlQuery& query, int directoryMediaArtField, int embeddedMediaArtField, const QString& userMediaArt)
    {
        if (!userMediaArt.isEmpty()) {
            return userMediaArt;
        }

        const QString directoryMediaArt(query.value(directoryMediaArtField).toString());
        const QString embeddedMediaArt(query.value(embeddedMediaArtField).toString());

        if (Settings::instance()->useDirectoryMediaArt()) {
            if (!directoryMediaArt.isEmpty()) {
                return directoryMediaArt;
            }
            return embeddedMediaArt;
        }
        if (!embeddedMediaArt.isEmpty()) {
            return embeddedMediaArt;
        }
        return directoryMediaArt;
    }

    QString mediaArtFromQuery(const QSqlQuery& query, int directoryMediaArtField, int embeddedMediaArtField, int userMediaArtField)
    {
        return mediaArtFromQuery(query, directoryMediaArtField, embeddedMediaArtField, query.value(userMediaArtField).toString());
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

    QString LibraryUtils::findMediaArtForDirectory(std::unordered_map<QString, QString>& mediaArtHash, const QString& directoryPath, const std::atomic_bool& cancelFlag)
    {
        {
            const auto found(mediaArtHash.find(directoryPath));
            if (found != mediaArtHash.end()) {
                return found->second;
            }
        }

        static const std::unordered_set<QString> suffixes{QLatin1String("jpeg"), QLatin1String("jpg"), QLatin1String("png")};
        QDirIterator iterator(directoryPath, QDir::Files | QDir::Readable);
        while (iterator.hasNext()) {
            if (cancelFlag) {
                return QString();
            }
            const QString filePath(iterator.next());
            const QFileInfo fileInfo(iterator.fileInfo());
            if (contains(suffixes, fileInfo.suffix().toLower())) {
                const QString baseName(fileInfo.completeBaseName().toLower());
                if (baseName == QLatin1String("cover") ||
                        baseName == QLatin1String("front") ||
                        baseName == QLatin1String("folder") ||
                        baseName.startsWith(QLatin1String("albumart"))) {
                    mediaArtHash.insert({directoryPath, filePath});
                    return filePath;
                }
                continue;
            }
        }

        mediaArtHash.insert({directoryPath, QString()});
        return QString();
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
                      "    artistId INTEGER NOT NULL REFERENCES artists(id) ON DELETE CASCADE"
                      ")")) {
                return;
            }

            if (!exec("CREATE TABLE tracks_albums ("
                      "    trackId INTEGER NOT NULL REFERENCES tracks(id) ON DELETE CASCADE,"
                      "    albumId INTEGER NOT NULL REFERENCES albums(id) ON DELETE CASCADE"
                      ")")) {
                return;
            }

            if (!exec("CREATE TABLE albums_artists ("
                      "    albumId INTEGER NOT NULL REFERENCES albums(id) ON DELETE CASCADE,"
                      "    artistId INTEGER NOT NULL REFERENCES artists(id) ON DELETE CASCADE"
                      ")")) {
                return;
            }

            if (!exec("CREATE TABLE tracks_genres ("
                      "    trackId INTEGER NOT NULL REFERENCES tracks(id) ON DELETE CASCADE,"
                      "    genreId INTEGER NOT NULL REFERENCES genres(id) ON DELETE CASCADE"
                      ")")) {
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

        auto runnable = new LibraryUpdateRunnable(mMediaArtDirectory);
        QObject::connect(runnable->notifier(), &LibraryUpdateRunnableNotifier::stageChanged, this, [this](LibraryUpdateRunnableNotifier::UpdateStage newStage) {
            mLibraryUpdateStage = newStage;
            emit updateStageChanged();
        });
        QObject::connect(runnable->notifier(), &LibraryUpdateRunnableNotifier::foundFilesChanged, this, [this](int found) {
            mFoundTracks = found;
            emit foundTracksChanged();
        });
        QObject::connect(runnable->notifier(), &LibraryUpdateRunnableNotifier::extractedFilesChanged, this, [this](int extracted) {
            mExtractedTracks = extracted;
            emit extractedTracksChanged();
        });
        QObject::connect(runnable->notifier(), &LibraryUpdateRunnableNotifier::finished, this, [this]() {
            mLibraryUpdateRunnable = nullptr;
            mLibraryUpdateStage = LibraryUpdateRunnableNotifier::NoneStage;
            mFoundTracks = 0;
            mExtractedTracks = 0;
            emit updatingChanged();
            emit updateStageChanged();
            emit foundTracksChanged();
            emit extractedTracksChanged();
            emit databaseChanged();
        });
        QObject::connect(qApp, &QCoreApplication::aboutToQuit, runnable->notifier(), [runnable]() { runnable->cancel(); });
        mLibraryUpdateRunnable = runnable;
        mLibraryUpdateStage = LibraryUpdateRunnableNotifier::PreparingStage;
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

        if (!QDir(mMediaArtDirectory).removeRecursively()) {
            qWarning() << "Failed to remove media art directory:" << mMediaArtDirectory;
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
        if (!mDatabaseInitialized) {
            return 0;
        }

        QSqlQuery query(Settings::instance()->useAlbumArtist() ? QLatin1String("SELECT COUNT(DISTINCT(albumArtist)) FROM tracks")
                                                               : QLatin1String("SELECT COUNT(DISTINCT(artist)) FROM tracks"));
        if (query.next()) {
            return query.value(0).toInt();
        }
        return 0;
    }

    int LibraryUtils::albumsCount() const
    {
        if (!mDatabaseInitialized) {
            return 0;
        }

        QSqlQuery query(QLatin1String("SELECT COUNT(DISTINCT(album)) FROM tracks"));
        if (query.next()) {
            return query.value(0).toInt();
        }
        return 0;
    }

    int LibraryUtils::tracksCount() const
    {
        if (!mDatabaseInitialized) {
            return 0;
        }

        QSqlQuery query(QLatin1String("SELECT COUNT(DISTINCT(id)) FROM tracks"));
        if (query.next()) {
            return query.value(0).toInt();
        }
        return 0;
    }

    int LibraryUtils::tracksDuration() const
    {
        if (!mDatabaseInitialized) {
            return 0;
        }

        QSqlQuery query(QLatin1String("SELECT SUM(duration) FROM (SELECT duration from tracks GROUP BY id)"));
        if (query.next()) {
            return query.value(0).toInt();
        }
        return 0;
    }

    QString LibraryUtils::randomMediaArt() const
    {
        if (!mDatabaseInitialized) {
            return QString();
        }

        QSqlQuery query(QLatin1String("SELECT directoryMediaArt, embeddedMediaArt FROM tracks "
                                      "WHERE directoryMediaArt != '' OR embeddedMediaArt != '' "
                                      "GROUP BY id ORDER BY RANDOM() LIMIT 1"));
        if (query.next()) {
            return mediaArtFromQuery(query, 0, 1);
        }
        return QString();
    }

    QString LibraryUtils::randomMediaArtForArtist(const QString& artist) const
    {
        if (!mDatabaseInitialized) {
            return QString();
        }

        QSqlQuery query;
        query.prepare(QLatin1String("SELECT directoryMediaArt, embeddedMediaArt FROM tracks "
                                    "WHERE (directoryMediaArt != '' OR embeddedMediaArt != '') AND artist = ? "
                                    "GROUP BY id "
                                    "ORDER BY RANDOM() LIMIT 1"));
        query.addBindValue(artist);
        query.exec();
        if (query.next()) {
            return mediaArtFromQuery(query, 0, 1);
        }
        return QString();
    }

    QString LibraryUtils::randomMediaArtForAlbum(const QString& artist, const QString& album) const
    {
        if (!mDatabaseInitialized) {
            return QString();
        }

        QSqlQuery query;
        query.prepare(QLatin1String("SELECT directoryMediaArt, embeddedMediaArt FROM tracks "
                                    "WHERE (directoryMediaArt != '' OR embeddedMediaArt != '') AND artist = ? AND album = ? "
                                    "GROUP BY id "
                                    "ORDER BY RANDOM() LIMIT 1"));
        query.addBindValue(artist);
        query.addBindValue(album);
        query.exec();
        if (query.next()) {
            return mediaArtFromQuery(query, 0, 1);
        }
        return QString();
    }

    QString LibraryUtils::randomMediaArtForGenre(const QString& genre) const
    {
        if (!mDatabaseInitialized) {
            return QString();
        }

        QSqlQuery query;
        query.prepare(QLatin1String("SELECT directoryMediaArt, embeddedMediaArt FROM tracks "
                                    "WHERE (directoryMediaArt != '' OR embeddedMediaArt != '') AND genre = ? "
                                    "GROUP BY id "
                                    "ORDER BY RANDOM() LIMIT 1"));
        query.addBindValue(genre);
        query.exec();
        if (query.next()) {
            return mediaArtFromQuery(query, 0, 1);
        }
        return QString();
    }

    void LibraryUtils::setMediaArt(const QString& artist, const QString& album, const QString& mediaArt)
    {
        if (!mDatabaseInitialized) {
            return;
        }

        if (!QDir().mkpath(mMediaArtDirectory)) {
            qWarning() << "failed to create media art directory:" << mMediaArtDirectory;
            return;
        }

        QString id(QUuid::createUuid().toString());
        id.remove(0, 1);
        id.chop(1);

        const QString newFilePath(QString::fromLatin1("%1/%2.%3")
                                  .arg(mMediaArtDirectory, id, QFileInfo(mediaArt).suffix()));

        if (!QFile::copy(mediaArt, newFilePath)) {
            qWarning() << "failed to copy file from" << mediaArt << "to" << newFilePath;
            return;
        }

        QSqlQuery query;
        query.prepare(QLatin1String("UPDATE tracks SET embeddedMediaArt = ? WHERE artist = ? AND album = ?"));
        query.addBindValue(newFilePath);
        query.addBindValue(artist);
        query.addBindValue(album);
        if (query.exec()) {
            emit mediaArtChanged();
        } else {
            qWarning() << "failed to update media art in the database:" << query.lastError();
        }
    }

    bool LibraryUtils::isUpdating() const
    {
        return mLibraryUpdateRunnable;
    }

    LibraryUpdateRunnableNotifier::UpdateStage LibraryUtils::updateStage() const
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

    void LibraryUtils::removeArtists(std::vector<QString>&& artists, bool deleteFiles)
    {
        if (mRemovingFiles || !mDatabaseInitialized) {
            return;
        }

        mRemovingFiles = true;
        emit removingFilesChanged();

        auto future = QtConcurrent::run([deleteFiles, artists = std::move(artists)] {
            const DatabaseConnectionGuard databaseGuard{removeFilesConnectionName};

            // Open database
            QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
            if (!db.isOpen()) {
                return;
            }

            const TransactionGuard transactionGuard(db);

            batchedCount(artists.size(), LibraryUtils::maxDbVariableCount, [&](size_t first, size_t count) {
                if (!qApp) {
                    return;
                }

                QString whereString(QLatin1String("WHERE artist = ?"));
                const QLatin1String wherePush(" OR artist = ?");
                whereString.reserve(whereString.size() + wherePush.size() * static_cast<int>(count - 1));
                for (size_t i = 1; i < count; ++i) {
                    whereString.push_back(wherePush);
                }
                if (deleteFiles) {
                    QSqlQuery query(db);
                    query.prepare(QLatin1String("SELECT filePath FROM tracks ") % whereString % QLatin1String(" GROUP BY id"));
                    for (size_t i = first, max = first + count; i < max; ++i) {
                        query.addBindValue(artists[i]);
                    }
                    if (query.exec()) {
                        while (query.next()) {
                            if (!qApp) {
                                return;
                            }
                            const QString filePath(query.value(0).toString());
                            if (!QFile::remove(filePath)) {
                                qWarning() << "failed to remove file:" << filePath;
                            }
                        }
                    } else {
                        qWarning() << "failed to select files from artists:" << query.lastError();
                    }
                }

                QSqlQuery query(db);
                query.prepare(QLatin1String("DELETE FROM tracks ") % whereString);
                for (size_t i = first, max = first + count; i < max; ++i) {
                    query.addBindValue(artists[i]);
                }
                if (!query.exec()) {
                    qWarning() << "failed to remove artists:" << query.lastError();
                }
            });
        });

        using Watcher = QFutureWatcher<void>;
        auto watcher = new Watcher(this);
        QObject::connect(watcher, &Watcher::finished, this, [this, watcher]() {
            mRemovingFiles = false;
            emit removingFilesChanged();
            emit databaseChanged();
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }

    void LibraryUtils::removeAlbums(std::vector<Album>&& albums, bool deleteFiles)
    {
        if (mRemovingFiles || !mDatabaseInitialized) {
            return;
        }

        mRemovingFiles = true;
        emit removingFilesChanged();

        auto future = QtConcurrent::run([deleteFiles, albums = std::move(albums)] {
            const DatabaseConnectionGuard databaseGuard{removeFilesConnectionName};

            // Open database
            QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
            if (!db.isOpen()) {
                return;
            }

            const TransactionGuard transactionGuard(db);

            batchedCount(albums.size(), LibraryUtils::maxDbVariableCount / 2, [&](size_t first, size_t count) {
                if (!qApp) {
                    return;
                }

                QString whereString(QLatin1String("WHERE (artist = ? AND album = ?)"));
                const QLatin1String wherePush(" OR (artist = ? AND album = ?)");
                whereString.reserve(whereString.size() + wherePush.size() * static_cast<int>(count - 1));
                for (size_t i = 1; i < count; ++i) {
                    whereString.push_back(wherePush);
                }
                if (deleteFiles) {
                    QSqlQuery query(db);
                    query.prepare(QLatin1String("SELECT filePath FROM tracks ") % whereString % QLatin1String(" GROUP BY id"));
                    for (size_t i = first, max = first + count; i < max; ++i) {
                        const Album& album = albums[i];
                        query.addBindValue(album.artist);
                        query.addBindValue(album.album);
                    }
                    if (query.exec()) {
                        while (query.next()) {
                            if (!qApp) {
                                return;
                            }

                            const QString filePath(query.value(0).toString());
                            if (!QFile::remove(filePath)) {
                                qWarning() << "failed to remove file:" << filePath;
                            }
                        }
                    } else {
                        qWarning() << "failed to select files from albums:" << query.lastError();
                    }
                }

                QSqlQuery query(db);
                query.prepare(QLatin1String("DELETE FROM tracks ") % whereString);
                for (size_t i = first, max = first + count; i < max; ++i) {
                    const Album& album = albums[i];
                    query.addBindValue(album.artist);
                    query.addBindValue(album.album);
                }
                if (!query.exec()) {
                    qWarning() << "failed to remove albums:" << query.lastError();
                }
            });
        });

        using Watcher = QFutureWatcher<void>;
        auto watcher = new Watcher(this);
        QObject::connect(watcher, &Watcher::finished, this, [this, watcher]() {
            mRemovingFiles = false;
            emit removingFilesChanged();
            emit databaseChanged();
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }

    void LibraryUtils::removeGenres(std::vector<QString>&& genres, bool deleteFiles)
    {
        if (mRemovingFiles || !mDatabaseInitialized) {
            return;
        }

        mRemovingFiles = true;
        emit removingFilesChanged();

        auto future = QtConcurrent::run([deleteFiles, genres = std::move(genres)] {
            const DatabaseConnectionGuard databaseGuard{removeFilesConnectionName};

            // Open database
            QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
            if (!db.isOpen()) {
                return;
            }

            const TransactionGuard transactionGuard(db);

            batchedCount(genres.size(), LibraryUtils::maxDbVariableCount, [&](size_t first, size_t count) {
                if (!qApp) {
                    return;
                }

                QString whereString(QLatin1String("WHERE genre = ?"));
                const QLatin1String wherePush(" OR genre = ?");
                whereString.reserve(whereString.size() + wherePush.size() * static_cast<int>(count - 1));
                for (size_t i = 1; i < count; ++i) {
                    whereString.push_back(wherePush);
                }
                if (deleteFiles) {
                    QSqlQuery query(db);
                    query.prepare(QLatin1String("SELECT filePath FROM tracks ") % whereString % QLatin1String(" GROUP BY id"));
                    for (size_t i = first, max = first + count; i < max; ++i) {
                        query.addBindValue(genres[i]);
                    }
                    if (query.exec()) {
                        while (query.next()) {
                            if (!qApp) {
                                return;
                            }
                            const QString filePath(query.value(0).toString());
                            if (!QFile::remove(filePath)) {
                                qWarning() << "failed to remove file:" << filePath;
                            }
                        }
                    } else {
                        qWarning() << "failed to select files from artists:" << query.lastError();
                    }
                }

                QSqlQuery query(db);
                query.prepare(QLatin1String("DELETE FROM tracks ") % whereString);
                for (size_t i = first, max = first + count; i < max; ++i) {
                    query.addBindValue(genres[i]);
                }
                if (!query.exec()) {
                    qWarning() << "failed to remove artists:" << query.lastError();
                }
            });
        });

        using Watcher = QFutureWatcher<void>;
        auto watcher = new Watcher(this);
        QObject::connect(watcher, &Watcher::finished, this, [this, watcher]() {
            mRemovingFiles = false;
            emit removingFilesChanged();
            emit databaseChanged();
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }

    void LibraryUtils::removeFiles(std::vector<QString>&& files, bool deleteFiles, bool canHaveDirectories)
    {
        if (mRemovingFiles || !mDatabaseInitialized) {
            return;
        }

        mRemovingFiles = true;
        emit removingFilesChanged();

        auto future = QtConcurrent::run([deleteFiles, canHaveDirectories, files = std::move(files)]() mutable {
            const DatabaseConnectionGuard databaseGuard{removeFilesConnectionName};

            QElapsedTimer timer;
            timer.start();

            // Open database
            QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
            if (!db.isOpen()) {
                return;
            }

            qInfo() << "db opening time:" << timer.restart();

            const TransactionGuard transactionGuard(db);

            batchedCount(files.size(), LibraryUtils::maxDbVariableCount, [&](size_t first, size_t count) {
                if (!qApp) {
                    return;
                }

                QString queryString(QLatin1String("DELETE FROM tracks WHERE "));

                const auto handleFile = [&](QString& filePath) {
                    if (canHaveDirectories && QFileInfo(filePath).isDir()) {
                        if (deleteFiles && !QDir(filePath).removeRecursively()) {
                            qWarning() << "failed to remove directory:" << filePath;
                        }
                        if (!filePath.endsWith(QLatin1Char('/'))) {
                            filePath.push_back(QLatin1Char('/'));
                        }
                        return QLatin1String("instr(filePath, ?) = 1");
                    }
                    if (deleteFiles && !QFile::remove(filePath)) {
                        qWarning() << "failed to remove file:" << filePath;
                    }
                    return QLatin1String("filePath = ?");
                };

                queryString.push_back(handleFile(files.front()));
                for (size_t i = first + 1, max = first + count; i < max; ++i) {
                    queryString.push_back(QLatin1String("OR "));
                    queryString.push_back(handleFile(files[i]));
                }

                QSqlQuery query(db);
                query.prepare(queryString);
                for (size_t i = first, max = first + count; i < max; ++i) {
                    query.addBindValue(files[i]);
                }
                if (!query.exec()) {
                    qWarning() << "failed to remove tracks:" << query.lastError();
                }
            });

            qInfo() << "file removing time:" << timer.elapsed();
        });

        using Watcher = QFutureWatcher<void>;
        auto watcher = new Watcher(this);
        QObject::connect(watcher, &Watcher::finished, this, [this, watcher]() {
            mRemovingFiles = false;
            emit removingFilesChanged();
            emit databaseChanged();
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }

    QString LibraryUtils::titleTag() const
    {
        return tagutils::TitleTag;
    }

    QString LibraryUtils::artistsTag() const
    {
        return tagutils::ArtistsTag;
    }

    QString LibraryUtils::albumArtistsTag() const
    {
        return tagutils::AlbumArtistsTag;
    }

    QString LibraryUtils::albumsTag() const
    {
        return tagutils::AlbumsTag;
    }

    QString LibraryUtils::yearTag() const
    {
        return tagutils::YearTag;
    }

    QString LibraryUtils::trackNumberTag() const
    {
        return tagutils::TrackNumberTag;
    }

    QString LibraryUtils::genresTag() const
    {
        return tagutils::GenresTag;
    }

    QString LibraryUtils::discNumberTag() const
    {
        return tagutils::DiscNumberTag;
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

        const QString mediaArtDirectory(mMediaArtDirectory);

        auto future = QtConcurrent::run([files, tags, incrementTrackNumber, mediaArtDirectory]() {
            qInfo("Start saving tags");

            QElapsedTimer timer;
            timer.start();

            QMimeDatabase mimeDb;

            if (!QDir().mkpath(mediaArtDirectory)) {
                qWarning() << "failed to create media art directory:" << mediaArtDirectory;
            }
            std::unordered_map<QByteArray, QString> embeddedMediaArtFiles(LibraryUtils::instance()->getEmbeddedMediaArt());

            std::vector<QString> embeddedMediaArt;
            embeddedMediaArt.reserve(static_cast<size_t>(files.size()));
            const auto callback = [&](tagutils::Info& info) {
                QString path(LibraryUtils::instance()->saveEmbeddedMediaArt(info.mediaArtData, embeddedMediaArtFiles, mimeDb));
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
                const QString directoryMediaArt(findMediaArtForDirectory(mediaArtDirectoriesHash, fileInfo.path(), false));
                adder.addTrackToDatabase(info.filePath, getLastModifiedTime(info.filePath), info, directoryMediaArt, embeddedMediaArt[i]);
            }

            qInfo("Done saving tags, %lldms", timer.elapsed());
        });

        using Watcher = QFutureWatcher<void>;
        auto watcher = new Watcher(this);
        QObject::connect(watcher, &Watcher::finished, this, [this, watcher]() {
            mSavingTags = false;
            emit savingTagsChanged();
            emit databaseChanged();
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }

    std::unordered_map<QByteArray, QString> LibraryUtils::getEmbeddedMediaArt()
    {
        std::unordered_map<QByteArray, QString> embeddedMediaArtFiles;
        const QFileInfoList files(QDir(mMediaArtDirectory).entryInfoList({QLatin1String("*-embedded.*")}, QDir::Files));
        embeddedMediaArtFiles.reserve(static_cast<size_t>(files.size()));
        for (const QFileInfo& info : files) {
            const QString baseName(info.baseName());
            const int index = baseName.indexOf(QStringLiteral("-embedded"));
            embeddedMediaArtFiles.emplace(baseName.left(index).toLatin1(), info.filePath());
        }
        return embeddedMediaArtFiles;
    }

    QString LibraryUtils::saveEmbeddedMediaArt(const QByteArray& data, std::unordered_map<QByteArray, QString>& embeddedMediaArtFiles, const QMimeDatabase& mimeDb)
    {
        if (data.isEmpty()) {
            return QString();
        }

        QByteArray md5(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex());
        {
            const auto found(embeddedMediaArtFiles.find(md5));
            if (found != embeddedMediaArtFiles.end()) {
                return found->second;
            }
        }

        const QString suffix(mimeDb.mimeTypeForData(data).preferredSuffix());
        if (suffix.isEmpty()) {
            return QString();
        }

        const QString filePath(QStringLiteral("%1/%2-embedded.%3")
                               .arg(mMediaArtDirectory, QString::fromLatin1(md5), suffix));
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(data);
            embeddedMediaArtFiles.emplace(std::move(md5), filePath);
            return filePath;
        }

        return QString();
    }

    LibraryUtils::LibraryUtils(QObject* parent)
        : QObject(parent),
          mDatabaseInitialized(false),
          mCreatedTables(false),
          mLibraryUpdateRunnable(nullptr),
          mLibraryUpdateStage(LibraryUpdateRunnableNotifier::NoneStage),
          mFoundTracks(0),
          mExtractedTracks(0),
          mRemovingFiles(false),
          mSavingTags(false),
          mMediaArtDirectory(QString::fromLatin1("%1/media-art").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)))
    {
        qRegisterMetaType<LibraryUpdateRunnableNotifier::UpdateStage>();
        initDatabase();
        QObject::connect(this, &LibraryUtils::databaseChanged, this, &LibraryUtils::mediaArtChanged);
    }
}

#include "libraryutils.moc"
