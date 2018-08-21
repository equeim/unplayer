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
#include <QUuid>
#include <QtConcurrentRun>

#include "settings.h"
#include "stdutils.h"
#include "tagutils.h"

namespace unplayer
{
    namespace
    {
        const QString rescanConnectionName(QLatin1String("unplayer_rescan"));

        const QLatin1String flacMimeType("audio/flac");

        const QLatin1String aacMimeType("audio/aac");
        const QLatin1String mp4MimeType("audio/mp4");
        const QLatin1String mp4bMimeType("audio/x-m4b");

        const QLatin1String mpegMimeType("audio/mpeg");

        const QLatin1String oggMimeType("audio/ogg");
        const QLatin1String vorbisOggMimeType("audio/x-vorbis+ogg");
        const QLatin1String flacOggMimeType("audio/x-flac+ogg");
        const QLatin1String opusOggMimeType("audio/x-opus+ogg");

        const QLatin1String apeMimeType("audio/x-ape");

        const QLatin1String matroskaMimeType("audio/x-matroska");
        const QLatin1String genericMatroskaMimeType("application/x-matroska");

        const QLatin1String wavMimeType("audio/x-wav");
        const QLatin1String wavpackMimeType("audio/x-wavpack");

        const QLatin1String mp4VideoMimeType("video/mp4");
        const QLatin1String mpegVideoMimeType("video/mpeg");
        const QLatin1String matroskaVideoMimeType("video/x-matroska");
        const QLatin1String oggVideoMimeType("video/ogg");


        std::unique_ptr<LibraryUtils> instancePointer;

        QString emptyIfNull(const QString& string)
        {
            if (string.isNull()) {
                return QStringLiteral("");
            }
            return string;
        }


        void updateTrackInDatabase(const QSqlDatabase& db,
                                   bool inDb,
                                   int id,
                                   const QFileInfo& fileInfo,
                                   const tagutils::Info& info,
                                   const QString& mediaArt)
        {
            if (inDb) {
                QSqlQuery query(db);
                query.prepare(QStringLiteral("DELETE FROM tracks WHERE id = ?"));
                query.addBindValue(id);
                if (!query.exec()) {
                    qWarning() << "failed to remove track from database" << query.lastQuery();
                }
            }

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
                QString queryString(QStringLiteral("INSERT INTO tracks (id, modificationTime, year, trackNumber, duration, filePath, title, artist, album, discNumber, genre, mediaArt) "
                                                   "VALUES "));
                const auto sizeOrOne = [](const QStringList& strings) {
                    return strings.empty() ? 1 : strings.size();
                };
                const int count = sizeOrOne(info.artists) * sizeOrOne(info.albums) * sizeOrOne(info.genres);
                for (int i = 0; i < count; ++i) {
                    queryString.push_back(QStringLiteral("(%1, %2, %3, %4, %5, ?, ?, ?, ?, ?, ?, ?)"));
                    if (i != (count - 1)) {
                        queryString.push_back(',');
                    }
                }
                queryString = queryString.arg(id).arg(fileInfo.lastModified().toMSecsSinceEpoch()).arg(info.year).arg(info.trackNumber).arg(info.duration);
                return queryString;
            }());

            forEachOrOnce(info.artists, [&](const QString& artist) {
                forEachOrOnce(info.albums, [&](const QString& album) {
                    forEachOrOnce(info.genres, [&](const QString& genre) {
                        query.addBindValue(fileInfo.filePath());
                        query.addBindValue(info.title);
                        query.addBindValue(emptyIfNull(artist));
                        query.addBindValue(emptyIfNull(album));
                        query.addBindValue(emptyIfNull(info.discNumber));
                        query.addBindValue(emptyIfNull(genre));
                        query.addBindValue(emptyIfNull(mediaArt));
                    });
                });
            });

            if (!query.exec()) {
                qWarning() << "failed to insert track in the database" << query.lastError();
            }
        }
    }

    MimeType mimeTypeFromString(const QString& string)
    {
        if (string == flacMimeType) {
            return MimeType::Flac;
        }
        if (string == mp4MimeType) {
            return MimeType::Mp4;
        }
        if (string == mp4bMimeType) {
            return MimeType::Mp4b;
        }
        if (string == mpegMimeType) {
            return MimeType::Mpeg;
        }
        if (string == vorbisOggMimeType) {
            return MimeType::VorbisOgg;
        }
        if (string == flacOggMimeType) {
            return MimeType::FlacOgg;
        }
        if (string == opusOggMimeType) {
            return MimeType::OpusOgg;
        }
        if (string == apeMimeType) {
            return MimeType::Ape;
        }
        if (string == genericMatroskaMimeType) {
            return MimeType::Matroska;
        }
        if (string == wavMimeType) {
            return MimeType::Wav;
        }
        if (string == wavpackMimeType) {
            return MimeType::Wavpack;
        }
        return MimeType::Other;
    }

    const std::unordered_set<QString> LibraryUtils::mimeTypesByExtension{flacMimeType,
                                                                         aacMimeType,
                                                                         mp4MimeType,
                                                                         mp4bMimeType,
                                                                         mpegMimeType,
                                                                         oggMimeType,
                                                                         apeMimeType,
                                                                         matroskaMimeType,
                                                                         wavMimeType,
                                                                         wavpackMimeType};

    const std::unordered_set<QString> LibraryUtils::mimeTypesByContent{flacMimeType,
                                                                       mp4MimeType,
                                                                       mp4bMimeType,
                                                                       mpegMimeType,
                                                                       vorbisOggMimeType,
                                                                       flacOggMimeType,
                                                                       opusOggMimeType,
                                                                       apeMimeType,
                                                                       genericMatroskaMimeType,
                                                                       wavMimeType,
                                                                       wavpackMimeType};

    const std::unordered_set<QString> LibraryUtils::videoMimeTypesByExtension{mp4VideoMimeType,
                                                                              mpegVideoMimeType,
                                                                              matroskaVideoMimeType,
                                                                              oggVideoMimeType};

    const QString LibraryUtils::databaseType(QLatin1String("QSQLITE"));

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

        const QDir dir(directoryPath);
        const QStringList found(dir.entryList(QDir::Files | QDir::Readable)
                                .filter(QRegularExpression(QStringLiteral("^(albumart.*|cover|folder|front)\\.(jpeg|jpg|png)$"),
                                                           QRegularExpression::CaseInsensitiveOption)));
        if (found.isEmpty()) {
            mediaArtHash.insert({directoryPath, QString()});
            return QString();
        }

        const QString mediaArt(dir.filePath(found.first()));
        mediaArtHash.insert({directoryPath, mediaArt});
        return mediaArt;
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
            static const std::vector<QString> fields{QLatin1String("id"),
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
                                                     QLatin1String("mediaArt")};

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
                                          "    mediaArt TEXT"
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
            qDebug() << "start scanning files";
            const QTime time(QTime::currentTime());
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

                // Library directories
                const auto prepareDirs = [](QStringList&& dirs) {
                    dirs.removeDuplicates();
                    for (QString& dir : dirs) {
                        if (!dir.endsWith('/')) {
                            dir.push_back('/');
                        }
                    }
                    return dirs;
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

                std::unordered_map<QString, bool> noMediaDirectories;
                const auto isNoMediaDirectory = [&noMediaDirectories](const QString& directory) {
                    {
                        const auto found(noMediaDirectories.find(directory));
                        if (found != noMediaDirectories.end()) {
                            return found->second;
                        }
                    }
                    const bool noMedia = QFileInfo(directory + QStringLiteral("/.nomedia")).isFile();
                    noMediaDirectories.insert({directory, noMedia});
                    return noMedia;
                };

                std::unordered_map<QString, int> files;
                int lastId = -1;
                std::unordered_map<int, long long> modificationTimeHash;
                std::unordered_map<int, QString> mediaArtHash;

                std::vector<int> filesToRemove;

                {
                    QSqlQuery query(QLatin1String("SELECT id, filePath, modificationTime, mediaArt FROM tracks GROUP BY id ORDER BY id"), db);
                    if (query.lastError().type() != QSqlError::NoError) {
                        qWarning() << "failed to get files from database" << query.lastError();
                        return;
                    }

                    std::unordered_map<QString, bool> mediaArtExistanceHash;

                    while (query.next()) {
                        const int id(query.value(0).toInt());
                        const QString filePath(query.value(1).toString());
                        const QFileInfo fileInfo(filePath);

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
                                remove = isNoMediaDirectory(fileInfo.absolutePath());
                            }
                        }

                        if (remove) {
                            filesToRemove.push_back(id);
                        } else {
                            files.insert({filePath, id});
                            modificationTimeHash.insert({id, query.value(2).toLongLong()});

                            const QString mediaArt(query.value(3).toString());
                            if (mediaArt.isEmpty()) {
                                mediaArtHash.insert({id, mediaArt});
                            } else {
                                // if media art is not empty but file doesn't exist, do not insert it in mediaArtHash
                                const auto found(mediaArtExistanceHash.find(mediaArt));
                                if (found == mediaArtExistanceHash.end()) {
                                    if (QFile::exists(mediaArt)) {
                                        mediaArtExistanceHash.insert({mediaArt, true});
                                        mediaArtHash.insert({id, mediaArt});
                                    } else {
                                        mediaArtExistanceHash.insert({mediaArt, false});
                                    }
                                } else if (found->second) {
                                    mediaArtHash.insert({id, mediaArt});
                                }
                            }
                        }
                    }
                }

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

                std::unordered_map<QString, QString> mediaArtDirectoriesHash;
                const QMimeDatabase mimeDb;
                const bool preferDirectoryMediaArt = Settings::instance()->useDirectoryMediaArt();

                for (const QString& topLevelDirectory : libraryDirectories) {
                    QDirIterator iterator(topLevelDirectory, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
                    while (iterator.hasNext()) {
                        if (!qApp) {
                            qWarning() << "app shutdown, stop updating";
                            db.commit();
                            return;
                        }

                        iterator.next();
                        const QString filePath(iterator.filePath());
                        const QFileInfo fileInfo(iterator.fileInfo());

                        if (fileInfo.isDir()) {
                            continue;
                        }

                        if (!fileInfo.isReadable()) {
                            continue;
                        }

                        const auto foundInDb(files.find(filePath));

                        if (foundInDb == files.end()) {
                            // File is not in database

                            if (isNoMediaDirectory(fileInfo.path())) {
                                continue;
                            }

                            if (isBlacklisted(filePath)) {
                                continue;
                            }

                            QString mimeType(mimeDb.mimeTypeForFile(filePath, QMimeDatabase::MatchExtension).name());
                            if (contains(mimeTypesByExtension, mimeType)) {
                                mimeType = mimeDb.mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name();
                                if (contains(mimeTypesByContent, mimeType)) {
                                    const tagutils::Info trackInfo(tagutils::getTrackInfo(fileInfo, mimeType));
                                    updateTrackInDatabase(db,
                                                          false,
                                                          ++lastId,
                                                          fileInfo,
                                                          trackInfo,
                                                          getTrackMediaArt(trackInfo.mediaArtData,
                                                                           embeddedMediaArtFiles,
                                                                           fileInfo,
                                                                           mediaArtDirectoriesHash,
                                                                           preferDirectoryMediaArt));
                                }
                            }
                        } else {
                            // File is in database

                            const int id = foundInDb->second;

                            const long long modificationTime = fileInfo.lastModified().toMSecsSinceEpoch();
                            if (modificationTime == modificationTimeHash[id]) {
                                // File has not changed

                                QString mediaArt;
                                bool deleted = false;
                                {
                                    const auto found(mediaArtHash.find(id));
                                    if (found == mediaArtHash.end()) {
                                        deleted = true;
                                    } else {
                                        mediaArt = found->second;
                                    }
                                }

                                const bool embeddedOrManual = mediaArt.startsWith(mMediaArtDirectory);
                                const bool embedded = embeddedOrManual && mediaArt.contains(QStringLiteral("-embedded"));
                                const bool manual = embeddedOrManual && !embedded;

                                if (manual) {
                                    continue;
                                }

                                if (!embedded || preferDirectoryMediaArt) {
                                    const QByteArray mediaArtData([&]() {
                                        // if media art was empty or embedded, we don't need to extract embedded
                                        if ((!deleted && mediaArt.isEmpty()) || embedded) {
                                            return QByteArray();
                                        }
                                        return tagutils::getTrackInfo(fileInfo,
                                                                      mimeDb.mimeTypeForFile(filePath,
                                                                                             QMimeDatabase::MatchContent).name()).mediaArtData;
                                    }());

                                    const QString newMediaArt(getTrackMediaArt(mediaArtData,
                                                                               embeddedMediaArtFiles,
                                                                               fileInfo,
                                                                               mediaArtDirectoriesHash,
                                                                               preferDirectoryMediaArt));
                                    // if media art was embedded and new is empty, do nothing
                                    if (!(embedded && newMediaArt.isEmpty()) && newMediaArt != mediaArt) {
                                        QSqlQuery query(db);
                                        query.prepare(QStringLiteral("UPDATE tracks SET mediaArt = ? WHERE id = ?"));
                                        query.addBindValue(emptyIfNull(newMediaArt));
                                        query.addBindValue(id);
                                        if (!query.exec()) {
                                            qWarning() << query.lastError();
                                        }
                                    }
                                }
                            } else {
                                // File has changed
                                const QString mimeType(mimeDb.mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name());
                                if (contains(mimeTypesByContent, mimeType)) {
                                    const tagutils::Info trackInfo(tagutils::getTrackInfo(fileInfo, mimeType));
                                    updateTrackInDatabase(db,
                                                          true,
                                                          id,
                                                          fileInfo,
                                                          trackInfo,
                                                          getTrackMediaArt(trackInfo.mediaArtData,
                                                                           embeddedMediaArtFiles,
                                                                           fileInfo,
                                                                           mediaArtDirectoriesHash,
                                                                           preferDirectoryMediaArt));
                                } else {
                                    filesToRemove.push_back(id);
                                }
                            }
                        }
                    }
                }

                if (!filesToRemove.empty()) {
                    qDebug() << "removing" << filesToRemove.size() << "tracks from database";
                    QString queryString(QLatin1String("DELETE FROM tracks WHERE id IN ("));
                    for (std::size_t i = 0, max = filesToRemove.size(); i < max; ++i) {
                        queryString.push_back(QString::number(filesToRemove[i]));
                        if (i != (max - 1)) {
                            queryString.push_back(',');
                        }
                    }
                    queryString.push_back(')');
                    QSqlQuery query(queryString, db);
                    if (query.lastError().type() != QSqlError::NoError) {
                        qWarning() << "failed to remove files from database" << query.lastQuery();
                    }
                }

                std::unordered_set<QString> allMediaArt;
                {
                    QSqlQuery query(QLatin1String("SELECT DISTINCT(mediaArt) FROM tracks WHERE mediaArt != ''"), db);
                    if (query.lastError().type() == QSqlError::NoError) {
                        if (query.last()) {
                            allMediaArt.reserve(query.at() + 1);
                            query.seek(-1);
                            while (query.next()) {
                                allMediaArt.insert(query.value(0).toString());
                            }
                        }
                    }
                }

                {
                    const QFileInfoList files(QDir(mMediaArtDirectory).entryInfoList(QDir::Files));
                    for (const QFileInfo& info : files) {
                        if (!contains(allMediaArt, info.filePath())) {
                            if (!QFile::remove(info.filePath())) {
                                qWarning() << "failed to remove file:" << info.filePath();
                            }
                        }
                    }
                }

                db.commit();
            }
            QSqlDatabase::removeDatabase(rescanConnectionName);
            qDebug() << "end scanning files" << time.msecsTo(QTime::currentTime());
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

        QSqlQuery query(QLatin1String("SELECT mediaArt FROM tracks WHERE mediaArt != '' GROUP BY mediaArt ORDER BY RANDOM() LIMIT 1"));
        if (query.next()) {
            return query.value(0).toString();
        }
        return QString();
    }

    QString LibraryUtils::randomMediaArtForArtist(const QString& artist)
    {
        if (!mDatabaseInitialized) {
            return QString();
        }

        QSqlQuery query;
        query.prepare(QLatin1String("SELECT mediaArt FROM tracks "
                                    "WHERE mediaArt != '' AND artist = ? "
                                    "GROUP BY mediaArt "
                                    "ORDER BY RANDOM() LIMIT 1"));
        query.addBindValue(artist);
        query.exec();
        if (query.next()) {
            return query.value(0).toString();
        }
        return QString();
    }

    QString LibraryUtils::randomMediaArtForAlbum(const QString& artist, const QString& album)
    {
        if (!mDatabaseInitialized) {
            return QString();
        }

        QSqlQuery query;
        query.prepare(QLatin1String("SELECT mediaArt FROM tracks "
                                    "WHERE mediaArt != '' AND artist = ? AND album = ? "
                                    "GROUP BY mediaArt "
                                    "ORDER BY RANDOM() LIMIT 1"));
        query.addBindValue(artist);
        query.addBindValue(album);
        query.exec();
        if (query.next()) {
            return query.value(0).toString();
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
        query.prepare(QLatin1String("UPDATE tracks SET mediaArt = ? WHERE artist = ? AND album = ?"));
        query.addBindValue(newFilePath);
        query.addBindValue(artist);
        query.addBindValue(album);
        if (query.exec()) {
            emit mediaArtChanged();
        } else {
            qWarning() << "failed to update media art in the database:" << query.lastError();
        }
    }

    void LibraryUtils::removeFileFromDatabase(const QString& filePath)
    {
        QSqlQuery query;
        query.prepare(QStringLiteral("DELETE FROM tracks WHERE filePath = ?"));
        query.addBindValue(filePath);
        if (!query.exec()) {
            qWarning() << "failed to remove file from database" << query.lastQuery();
        }
    }

    void LibraryUtils::removeFilesFromDatabase(const QStringList &files)
    {
        auto db = QSqlDatabase::database();
        db.transaction();
        for (const QString& filePath : files) {
            removeFileFromDatabase(filePath);
        }
        db.commit();
        emit databaseChanged();
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

    QString LibraryUtils::getTrackMediaArt(const QByteArray& embeddedMediaArtData,
                                           std::unordered_map<QByteArray, QString>& embeddedMediaArtFiles,
                                           const QFileInfo& fileInfo,
                                           std::unordered_map<QString, QString>& mediaArtDirectories,
                                           bool preferDirectoriesMediaArt)
    {
        QString mediaArt;
        if (preferDirectoriesMediaArt) {
            mediaArt = findMediaArtForDirectory(mediaArtDirectories, fileInfo.path());
            if (mediaArt.isEmpty()) {
                if (!embeddedMediaArtData.isEmpty()) {
                    mediaArt = saveEmbeddedMediaArt(embeddedMediaArtData, embeddedMediaArtFiles);
                }
            }
        } else {
            if (embeddedMediaArtData.isEmpty()) {
                mediaArt = findMediaArtForDirectory(mediaArtDirectories, fileInfo.path());
            } else {
                mediaArt = saveEmbeddedMediaArt(embeddedMediaArtData, embeddedMediaArtFiles);
            }
        }
        return mediaArt;
    }

    QString LibraryUtils::saveEmbeddedMediaArt(const QByteArray& data, std::unordered_map<QByteArray, QString>& embeddedMediaArtFiles)
    {
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
