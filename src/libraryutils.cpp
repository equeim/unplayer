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

#include "libraryutils.h"

#include <functional>
#include <memory>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QQmlEngine>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QUuid>
#include <QtConcurrentRun>

#include "settings.h"
#include "stdutils.h"
#include "tagutils.h"
#include "utilsfunctions.h"

namespace unplayer
{
    namespace
    {
        const QString rescanConnectionName(QLatin1String("unplayer_rescan"));

        const QLatin1String flacSuffix("flac");
        const QLatin1String aacSuffix("aac");

        const QLatin1String m4aSuffix("m4a");
        const QLatin1String f4aSuffix("f4a");
        const QLatin1String m4bSuffix("m4b");
        const QLatin1String f4bSuffix("f4b");

        const QLatin1String mp3Suffix("mp3");
        const QLatin1String mpgaSuffix("mpga");

        const QLatin1String ogaSuffix("oga");
        const QLatin1String oggSuffix("ogg");
        const QLatin1String opusSuffix("opus");

        const QLatin1String apeSuffix("ape");

        const QLatin1String mkaSuffix("mka");

        const QLatin1String wavSuffix("wav");

        const QLatin1String wvSuffix("wv");
        const QLatin1String wvpSuffix("wvp");


        std::unique_ptr<LibraryUtils> instancePointer;

        inline QString emptyIfNull(const QString& string)
        {
            if (string.isNull()) {
                return QString("");
            }
            return string;
        }


        void addTrackToDatabase(const QSqlDatabase& db,
                                int id,
                                const QString& filePath,
                                long long modificationTime,
                                const tagutils::Info& info,
                                const QString& directoryMediaArt,
                                const QString& embeddedMediaArt)
        {
            const auto forEachOrOnce = [](const QStringList& strings, const std::function<void(const QString&)>& exec) {
                if (strings.empty()) {
                    exec(QString());
                } else {
                    for (const QString& string : strings) {
                        exec(string);
                    }
                }
            };

            QSqlQuery query(db);
            query.prepare([&]() {
                QString queryString(QStringLiteral("INSERT INTO tracks (id, modificationTime, year, trackNumber, duration, filePath, title, artist, album, discNumber, genre, directoryMediaArt, embeddedMediaArt) "
                                                   "VALUES "));
                const auto sizeOrOne = [](const QStringList& strings) {
                    return strings.empty() ? 1 : strings.size();
                };
                const int count = sizeOrOne(info.artists) * sizeOrOne(info.albums) * sizeOrOne(info.genres);
                for (int i = 0; i < count; ++i) {
                    queryString.push_back(QStringLiteral("(%1, %2, %3, %4, %5, ?, ?, ?, ?, ?, ?, ?, ?)"));
                    if (i != (count - 1)) {
                        queryString.push_back(QLatin1Char(','));
                    }
                }
                queryString = queryString.arg(id).arg(modificationTime).arg(info.year).arg(info.trackNumber).arg(info.duration);
                return queryString;
            }());

            forEachOrOnce(info.artists, [&](const QString& artist) {
                forEachOrOnce(info.albums, [&](const QString& album) {
                    forEachOrOnce(info.genres, [&](const QString& genre) {
                        query.addBindValue(filePath);
                        query.addBindValue(info.title);
                        query.addBindValue(emptyIfNull(artist));
                        query.addBindValue(emptyIfNull(album));
                        query.addBindValue(emptyIfNull(info.discNumber));
                        query.addBindValue(emptyIfNull(genre));
                        query.addBindValue(emptyIfNull(directoryMediaArt));
                        query.addBindValue(emptyIfNull(embeddedMediaArt));
                    });
                });
            });

            if (!query.exec()) {
                qWarning() << "failed to insert track in the database" << query.lastError();
            }
        }
    }

    Extension extensionFromSuffux(const QString& suffix)
    {
        static const std::unordered_map<QString, Extension> extensions{
            {flacSuffix, Extension::FLAC},
            {aacSuffix, Extension::AAC},

            {m4aSuffix, Extension::M4A},
            {f4aSuffix, Extension::M4A},
            {m4bSuffix, Extension::M4A},
            {f4bSuffix, Extension::M4A},

            {mp3Suffix, Extension::MP3},
            {mpgaSuffix, Extension::MP3},

            {ogaSuffix, Extension::OGA},
            {oggSuffix, Extension::OGG},
            {opusSuffix, Extension::OPUS},

            {apeSuffix, Extension::APE},

            {mkaSuffix, Extension::MKA},

            {wavSuffix, Extension::WAV},

            {wvSuffix, Extension::WAVPACK},
            {wvpSuffix, Extension::WAVPACK}
        };
        static const auto end(extensions.end());

        const auto found(extensions.find(suffix));
        if (found == end) {
            return Extension::Other;
        }
        return found->second;
    }

    QString mediaArtFromQuery(const QSqlQuery& query, int directoryMediaArtField, int embeddedMediaArtField)
    {
        const QString embeddedMediaArt(query.value(embeddedMediaArtField).toString());

        if (!embeddedMediaArt.isEmpty() && !embeddedMediaArt.contains(QLatin1String("-embedded"))) {
            // Manual
            return embeddedMediaArt;
        }

        const QString directoryMediaArt(query.value(directoryMediaArtField).toString());

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

    const std::unordered_set<QString> LibraryUtils::mimeTypesExtensions{flacSuffix,
                                                                        aacSuffix,
                                                                        m4aSuffix,
                                                                        f4aSuffix,
                                                                        m4bSuffix,
                                                                        f4bSuffix,
                                                                        mp3Suffix,
                                                                        mpgaSuffix,
                                                                        ogaSuffix,
                                                                        oggSuffix,
                                                                        opusSuffix,
                                                                        apeSuffix,
                                                                        mkaSuffix,
                                                                        wavSuffix,
                                                                        wvSuffix,
                                                                        wvpSuffix};

    const std::unordered_set<QString> LibraryUtils::videoMimeTypesExtensions{QLatin1String("mp4"),
                                                                             QLatin1String("m4v"),
                                                                             QLatin1String("f4v"),
                                                                             QLatin1String("lrv"),

                                                                             QLatin1String("mpeg"),
                                                                             QLatin1String("mpg"),
                                                                             QLatin1String("mp2"),
                                                                             QLatin1String("mpe"),
                                                                             QLatin1String("vob"),

                                                                             QLatin1String("mkv"),

                                                                             QLatin1String("ogv")};

    const QString LibraryUtils::databaseType(QLatin1String("QSQLITE"));
    const int LibraryUtils::maxDbVariableCount = 999; // SQLITE_MAX_VARIABLE_NUMBER

    LibraryUtils* LibraryUtils::instance()
    {
        if (!instancePointer) {
            instancePointer.reset(new LibraryUtils());
            QQmlEngine::setObjectOwnership(instancePointer.get(), QQmlEngine::CppOwnership);
        }
        return instancePointer.get();
    }

    const QString& LibraryUtils::databaseFilePath()
    {
        return mDatabaseFilePath;
    }

    QString LibraryUtils::findMediaArtForDirectory(std::unordered_map<QString, QString>& mediaArtHash, const QString& directoryPath)
    {
        {
            const auto found(mediaArtHash.find(directoryPath));
            if (found != mediaArtHash.end()) {
                return found->second;
            }
        }

        static const QStringList nameFilters{QLatin1String("*.jpeg"), QLatin1String("*.jpg"), QLatin1String("*.png")};
        QDirIterator iterator(directoryPath, nameFilters, QDir::Files | QDir::Readable);
        while (iterator.hasNext()) {
            const QString filePath(iterator.next());
            const QString fileName(iterator.fileName().toLower());
            if (fileName == QLatin1String("cover") ||
                    fileName == QLatin1String("front") ||
                    fileName == QLatin1String("folder") ||
                    fileName.startsWith(QLatin1String("albumart"))) {
                mediaArtHash.insert({directoryPath, filePath});
                return filePath;
            }
        }

        mediaArtHash.insert({directoryPath, QString()});
        return QString();
    }

    void LibraryUtils::initDatabase()
    {
        qDebug() << "init db";

        const QString dataDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
        if (!QDir().mkpath(dataDir)) {
            qWarning() << "failed to create data directory";
            return;
        }

        auto db = QSqlDatabase::addDatabase(databaseType);
        db.setDatabaseName(mDatabaseFilePath);
        if (!db.open()) {
            qWarning() << "failed to open database:" << db.lastError();
            return;
        }

        bool createTable = !db.tables().contains(QLatin1String("tracks"));
        if (!createTable) {
            static const std::vector<QLatin1String> fields{QLatin1String("id"),
                                                           QLatin1String("filePath"),
                                                           QLatin1String("modificationTime"),
                                                           QLatin1String("title"),
                                                           QLatin1String("artist"),
                                                           QLatin1String("album"),
                                                           QLatin1String("year"),
                                                           QLatin1String("trackNumber"),
                                                           QLatin1String("discNumber"),
                                                           QLatin1String("genre"),
                                                           QLatin1String("duration"),
                                                           QLatin1String("directoryMediaArt"),
                                                           QLatin1String("embeddedMediaArt")};

            const QSqlRecord record(db.record(QLatin1String("tracks")));
            if (record.count() == static_cast<int>(fields.size())) {
                for (int i = 0, max = record.count(); i < max; ++i) {
                    if (!contains(fields, record.fieldName(i))) {
                        createTable = true;
                    }
                }
            } else {
                createTable = true;
            }

            if (createTable) {
                QSqlQuery query(QLatin1String("DROP TABLE tracks"));
                if (query.lastError().type() != QSqlError::NoError) {
                    qWarning() << "failed to remove table:" << query.lastError();
                }
            }
        }

        if (createTable) {
            QSqlQuery query(QLatin1String("CREATE TABLE tracks ("
                                          "    id INTEGER,"
                                          "    filePath TEXT,"
                                          "    modificationTime INTEGER,"
                                          "    title TEXT COLLATE NOCASE,"
                                          "    artist TEXT COLLATE NOCASE,"
                                          "    album TEXT COLLATE NOCASE,"
                                          "    year INTEGER,"
                                          "    trackNumber INTEGER,"
                                          "    discNumber TEXT,"
                                          "    genre TEXT,"
                                          "    duration INTEGER,"
                                          "    directoryMediaArt TEXT,"
                                          "    embeddedMediaArt TEXT"
                                          ")"));
            if (query.lastError().type() != QSqlError::NoError) {
                qWarning() << "failed to create table:" << query.lastError();
                return;
            }

            mCreatedTable = true;
        }

        mDatabaseInitialized = true;
    }

    void LibraryUtils::updateDatabase()
    {
        if (mUpdating) {
            return;
        }

        mUpdating = true;
        emit updatingChanged();

        const QFuture<void> future(QtConcurrent::run([=]() {
            qInfo("Start updating database");
            QElapsedTimer timer;
            timer.start();
            QElapsedTimer stageTimer;
            stageTimer.start();
            {
                // Open database
                auto db = QSqlDatabase::addDatabase(databaseType, rescanConnectionName);
                db.setDatabaseName(mDatabaseFilePath);
                if (!db.open()) {
                    QSqlDatabase::removeDatabase(rescanConnectionName);
                    qWarning() << "failed to open database" << db.lastError();
                    return;
                }
                db.transaction();

                // Create media art directory
                if (!QDir().mkpath(mMediaArtDirectory)) {
                    qWarning() << "failed to create media art directory:" << mMediaArtDirectory;
                }

                struct FileToAdd
                {
                    QString filePath;
                    QString directoryMediaArt;
                    int id;
                };
                std::vector<FileToAdd> filesToAdd;

                std::unordered_map<QByteArray, QString> embeddedMediaArtFiles;
                {
                    const QFileInfoList files(QDir(mMediaArtDirectory).entryInfoList({QLatin1String("*-embedded.*")}, QDir::Files));
                    embeddedMediaArtFiles.reserve(files.size());
                    for (const QFileInfo& info : files) {
                        const QString baseName(info.baseName());
                        const int index = baseName.indexOf(QStringLiteral("-embedded"));
                        embeddedMediaArtFiles.insert({baseName.left(index).toLatin1(), info.filePath()});
                    }
                }

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
                    int lastId = -1;
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

                        QSqlQuery query(QLatin1String("SELECT id, filePath, modificationTime, directoryMediaArt, embeddedMediaArt FROM tracks GROUP BY id ORDER BY id"), db);
                        if (query.lastError().type() != QSqlError::NoError) {
                            qWarning() << "failed to get files from database" << query.lastError();
                            return;
                        }

                        while (query.next()) {
                            const int id(query.value(0).toInt());
                            QString filePath(query.value(1).toString());
                            const QFileInfo fileInfo(filePath);
                            QString directory(fileInfo.path());

                            lastId = id;

                            bool remove = false;
                            if (!fileInfo.exists() || fileInfo.isDir() || !fileInfo.isReadable()) {
                                remove = true;
                            } else {
                                remove = true;
                                for (const QString& directory : libraryDirectories) {
                                    if (filePath.startsWith(directory)) {
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

                    qInfo("Files in database: %zd (took %.3f s)", filesInDb.size(), stageTimer.restart() / 1000.0);
                    qInfo("Files to remove: %zd", filesToRemove.size());

                    std::unordered_map<QString, QString> mediaArtDirectoriesHash;

                    const QStringList filters([&]() {
                        QStringList f;
                        f.reserve(mimeTypesExtensions.size());
                        for (const QString& ext : mimeTypesExtensions) {
                            f.push_back(QStringLiteral("*.") % ext);
                        }
                        return f;
                    }());

                    QString directory;
                    QString directoryMediaArt;

                    qInfo("Start scanning filesystem");
                    for (const QString& topLevelDirectory : libraryDirectories) {
                        QDirIterator iterator(topLevelDirectory, filters, QDir::Files | QDir::Readable, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
                        while (iterator.hasNext()) {
                            if (!qApp) {
                                qWarning() << "app shutdown, stop updating";
                                db.commit();
                                return;
                            }

                            QString filePath(iterator.next());
                            const QFileInfo fileInfo(iterator.fileInfo());

                            if (fileInfo.path() != directory) {
                                directory = fileInfo.path();
                                directoryMediaArt = findMediaArtForDirectory(mediaArtDirectoriesHash, directory);

                                const auto directoryMediaArtInDb(mediaArtDirectoriesInDbHash.find(directory));
                                if (directoryMediaArtInDb != mediaArtDirectoriesInDbHashEnd) {
                                    if (directoryMediaArtInDb->second != directoryMediaArt) {
                                        QSqlQuery query(db);
                                        query.prepare(QStringLiteral("UPDATE tracks SET directoryMediaArt = ? WHERE instr(filePath, ?) = 1"));
                                        query.addBindValue(emptyIfNull(directoryMediaArt));
                                        query.addBindValue(emptyIfNull(directory % QLatin1Char('/')));
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

                                filesToAdd.push_back({std::move(filePath), directoryMediaArt, ++lastId});
                            } else {
                                // File is in database

                                const FileInDb& file = foundInDb->second;

                                const long long modificationTime = getLastModifiedTime(filePath);
                                if (modificationTime == file.modificationTime) {
                                    // File has not changed
                                    if (file.embeddedMediaArtDeleted) {
                                        const QString embeddedMediaArt(saveEmbeddedMediaArt(tagutils::getTrackInfo(fileInfo, mimeDb).mediaArtData,
                                                                                            embeddedMediaArtFiles));
                                        QSqlQuery query(db);
                                        query.prepare(QStringLiteral("UPDATE tracks SET embeddedMediaArt = ? WHERE id = ?"));
                                        query.addBindValue(emptyIfNull(embeddedMediaArt));
                                        query.addBindValue(file.id);
                                        if (!query.exec()) {
                                            qWarning() << query.lastError();
                                        }
                                    }
                                } else {
                                    // File has changed
                                    filesToRemove.push_back(file.id);
                                    filesToAdd.push_back({foundInDb->first, directoryMediaArt, file.id});
                                }
                            }
                        }
                    }

                    qInfo("End scanning filesystem (took %.3f s), need to extract tags from %zd files", stageTimer.restart() / 1000.0, filesToAdd.size());

                    if (!filesToRemove.empty()) {
                        forMaxCountInRange(filesToRemove.size(), maxDbVariableCount, [&](int first, int count) {
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

                        qInfo("Removed %zd tracks from database (took %.3f s)", filesToRemove.size(), stageTimer.restart() / 1000.0);
                    }
                }

                if (!filesToAdd.empty()) {
                    qInfo("Start extracting tags from files");
                    int count = 0;
                    for (FileToAdd& file : filesToAdd) {
                        QFileInfo fileInfo(file.filePath);
                        const tagutils::Info trackInfo(tagutils::getTrackInfo(fileInfo, mimeDb));
                        if (trackInfo.isValid) {
                            ++count;
                            addTrackToDatabase(db,
                                               file.id,
                                               file.filePath,
                                               getLastModifiedTime(file.filePath),
                                               trackInfo,
                                               file.directoryMediaArt,
                                               saveEmbeddedMediaArt(trackInfo.mediaArtData, embeddedMediaArtFiles));
                        }
                    }
                    qInfo("Added %d tracks to database (took %.3f s)", count, stageTimer.restart() / 1000.0);
                }

                std::unordered_set<QString> allEmbeddedMediaArt;
                {
                    QSqlQuery query(QLatin1String("SELECT DISTINCT(embeddedMediaArt) FROM tracks WHERE embeddedMediaArt != ''"), db);
                    if (query.lastError().type() == QSqlError::NoError) {
                        if (query.last()) {
                            if (query.at() > 0) {
                                allEmbeddedMediaArt.reserve(query.at() + 1);
                            }
                            query.seek(QSql::BeforeFirstRow);
                            while (query.next()) {
                                allEmbeddedMediaArt.insert(query.value(0).toString());
                            }
                        }
                    }
                }

                {
                    const QFileInfoList files(QDir(mMediaArtDirectory).entryInfoList(QDir::Files));
                    for (const QFileInfo& info : files) {
                        if (!contains(allEmbeddedMediaArt, info.filePath())) {
                            if (!QFile::remove(info.filePath())) {
                                qWarning() << "failed to remove file:" << info.filePath();
                            }
                        }
                    }
                }

                db.commit();
            }
            QSqlDatabase::removeDatabase(rescanConnectionName);
            qInfo("End updating database (last stage took %.3f s)", stageTimer.elapsed() / 1000.0);
            qInfo("Total time: %.3f s", timer.elapsed() / 1000.0);
        }));

        auto watcher = new QFutureWatcher<void>(this);
        QObject::connect(watcher, &QFutureWatcher<void>::finished, this, [=]() {
            mUpdating = false;
            emit updatingChanged();
            emit databaseChanged();
        });
        watcher->setFuture(future);
    }

    void LibraryUtils::resetDatabase()
    {
        QSqlQuery query(QLatin1String("DELETE from tracks"));
        if (query.lastError().type() != QSqlError::NoError) {
            qWarning() << "failed to reset database";
        }
        if (!QDir(mMediaArtDirectory).removeRecursively()) {
            qWarning() << "failed to remove media art directory";
        }
        emit databaseChanged();
    }

    bool LibraryUtils::isDatabaseInitialized()
    {
        return mDatabaseInitialized;
    }

    bool LibraryUtils::isCreatedTable()
    {
        return mCreatedTable;
    }

    bool LibraryUtils::isUpdating()
    {
        return mUpdating;
    }

    int LibraryUtils::artistsCount()
    {
        if (!mDatabaseInitialized) {
            return 0;
        }

        QSqlQuery query(QLatin1String("SELECT COUNT(DISTINCT(artist)) FROM tracks"));
        if (query.next()) {
            return query.value(0).toInt();
        }
        return 0;
    }

    int LibraryUtils::albumsCount()
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

    int LibraryUtils::tracksCount()
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

    int LibraryUtils::tracksDuration()
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

    QString LibraryUtils::randomMediaArt()
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

    QString LibraryUtils::randomMediaArtForArtist(const QString& artist)
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

    QString LibraryUtils::randomMediaArtForAlbum(const QString& artist, const QString& album)
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

    QString LibraryUtils::randomMediaArtForGenre(const QString& genre)
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

    LibraryUtils::LibraryUtils()
        : mDatabaseInitialized(false),
          mCreatedTable(false),
          mUpdating(false),
          mDatabaseFilePath(QString::fromLatin1("%1/library.sqlite").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation))),
          mMediaArtDirectory(QString::fromLatin1("%1/media-art").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)))
    {
        initDatabase();
        QObject::connect(this, &LibraryUtils::databaseChanged, this, &LibraryUtils::mediaArtChanged);
    }

    QString LibraryUtils::saveEmbeddedMediaArt(const QByteArray& data, std::unordered_map<QByteArray, QString>& embeddedMediaArtFiles)
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

        const QString suffix(mMimeDb.mimeTypeForData(data).preferredSuffix());
        if (suffix.isEmpty()) {
            return QString();
        }

        const QString filePath(QStringLiteral("%1/%2-embedded.%3")
                               .arg(mMediaArtDirectory, QString::fromLatin1(md5), suffix));
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(data);
            embeddedMediaArtFiles.insert({std::move(md5), filePath});
            return filePath;
        }

        return QString();
    }
}
