/*
 * Unplayer
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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


        std::unique_ptr<LibraryUtils> instancePointer;


        void removeTrackFromDatabase(const QSqlDatabase& db, int id)
        {
            QSqlQuery query(db);
            query.prepare(QStringLiteral("DELETE FROM tracks WHERE id = ?"));
            query.addBindValue(id);
            if (!query.exec()) {
                qWarning() << "failed to remove file from database" << query.lastQuery();
            }
        }

        void updateTrackInDatabase(const QSqlDatabase& db,
                                   bool inDb,
                                   int id,
                                   const QFileInfo& fileInfo,
                                   const tagutils::Info& info,
                                   const QString& mediaArt)
        {
            if (inDb) {
                removeTrackFromDatabase(db, id);
            }

            const auto forEachOrOnce = [](const QStringList& strings, const std::function<void(const QString&)>& exec) {
                if (strings.isEmpty()) {
                    exec(QString());
                } else {
                    for (const QString& string : strings) {
                        exec(string);
                    }
                }
            };

            forEachOrOnce(info.artists, [&](const QString& artist) {
                forEachOrOnce(info.albums, [&](const QString& album) {
                    forEachOrOnce(info.genres, [&](const QString& genre) {
                        QSqlQuery query(db);
                        query.prepare(QStringLiteral("INSERT INTO tracks (id, filePath, modificationTime, title, artist, album, year, trackNumber, genre, duration, mediaArt) "
                                                     "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
                        query.addBindValue(id);
                        query.addBindValue(fileInfo.filePath());
                        query.addBindValue(fileInfo.lastModified().toMSecsSinceEpoch());
                        query.addBindValue(info.title);

                        if (artist.isNull()) {
                            // Empty string instead of NULL
                            query.addBindValue(QString(""));
                        } else {
                            query.addBindValue(artist);
                        }

                        if (album.isNull()) {
                            // Empty string instead of NULL
                            query.addBindValue(QString(""));
                        } else {
                            query.addBindValue(album);
                        }

                        query.addBindValue(info.year);
                        query.addBindValue(info.trackNumber);

                        if (genre.isNull()) {
                            // Empty string instead of NULL
                            query.addBindValue(QString(""));
                        } else {
                            query.addBindValue(genre);
                        }

                        query.addBindValue(info.duration);

                        if (mediaArt.isEmpty()) {
                            // Empty string instead of NULL
                            query.addBindValue(QString(""));
                        } else {
                            query.addBindValue(mediaArt);
                        }

                        if (!query.exec()) {
                            qWarning() << "failed to insert file in the database" << query.lastError();
                        }
                    });
                });
            });
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

    const QVector<QString> LibraryUtils::mimeTypesByExtension{flacMimeType,
                                                              aacMimeType,
                                                              mp4MimeType,
                                                              mp4bMimeType,
                                                              mpegMimeType,
                                                              oggMimeType,
                                                              apeMimeType,
                                                              matroskaMimeType,
                                                              wavMimeType,
                                                              wavpackMimeType};

    const QVector<QString> LibraryUtils::mimeTypesByContent{flacMimeType,
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

    QString LibraryUtils::findMediaArtForDirectory(QHash<QString, QString>& directoriesHash, const QString& directoryPath)
    {
        if (directoriesHash.contains(directoryPath)) {
            return directoriesHash.value(directoryPath);
        }

        const QDir dir(directoryPath);
        const QStringList found(dir.entryList(QDir::Files | QDir::Readable)
                                .filter(QRegularExpression(QStringLiteral("^(albumart.*|cover|folder|front)\\.(jpeg|jpg|png)$"),
                                                           QRegularExpression::CaseInsensitiveOption)));
        if (!found.isEmpty()) {
            const QString& mediaArt = dir.filePath(found.first());
            directoriesHash.insert(directoryPath, mediaArt);
            return mediaArt;
        }
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

        auto db = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"));
        db.setDatabaseName(mDatabaseFilePath);
        if (!db.open()) {
            qWarning() << "failed to open database:" << db.lastError();
            return;
        }

        bool createTable = !db.tables().contains(QLatin1String("tracks"));
        if (!createTable) {
            static const QVector<QString> fields{QLatin1String("id"),
                                                 QLatin1String("filePath"),
                                                 QLatin1String("modificationTime"),
                                                 QLatin1String("title"),
                                                 QLatin1String("artist"),
                                                 QLatin1String("album"),
                                                 QLatin1String("year"),
                                                 QLatin1String("trackNumber"),
                                                 QLatin1String("genre"),
                                                 QLatin1String("duration"),
                                                 QLatin1String("mediaArt")};

            const QSqlRecord record(db.record(QLatin1String("tracks")));
            if (record.count() == fields.size()) {
                for (int i = 0, max = record.count(); i < max; ++i) {
                    if (!fields.contains(record.fieldName(i))) {
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
            {
                auto db = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"), rescanConnectionName);
                db.setDatabaseName(mDatabaseFilePath);
                if (!db.open()) {
                    QSqlDatabase::removeDatabase(rescanConnectionName);
                    qWarning() << "failed to open database" << db.lastError();
                    return;
                }

                db.transaction();


                // Get all files from database
                QStringList files;
                QList<int> ids;
                QHash<int, long long> modificationTimeHash;
                QHash<int, QString> mediaArtHash;
                {
                    QSqlQuery query(QLatin1String("SELECT id, filePath, modificationTime, mediaArt FROM tracks GROUP BY id ORDER BY id"), db);
                    if (query.lastError().type() != QSqlError::NoError) {
                        qWarning() << "failed to get files from database" << query.lastError();
                        return;
                    }
                    while (query.next()) {
                        const int id(query.value(0).toInt());
                        files.append(query.value(1).toString());
                        ids.append(id);
                        modificationTimeHash.insert(id, query.value(2).toLongLong());
                        mediaArtHash.insert(id, query.value(3).toString());
                    }
                }

                QStringList libraryDirectories(Settings::instance()->libraryDirectories());
                libraryDirectories.removeDuplicates();

                if (!QDir().mkpath(mMediaArtDirectory)) {
                    qWarning() << "failed to create media art directory:" << mMediaArtDirectory;
                }

                QHash<QString, bool> noMediaDirectories;

                // Remove deleted files and files that are not in selected library directories
                for (int i = 0, max = files.size(); i < max; ++i) {
                    const QString& filePath = files.at(i);
                    const int id = ids.at(i);

                    const QFileInfo fileInfo(filePath);
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
                            const QString directory(fileInfo.path());
                            if (noMediaDirectories.contains(directory)) {
                                if (noMediaDirectories.value(directory)) {
                                    remove = true;
                                }
                            } else {
                                if (QFileInfo(QDir(directory).filePath(QLatin1String(".nomedia"))).isFile()) {
                                    noMediaDirectories.insert(directory, true);
                                    remove = true;
                                } else {
                                    noMediaDirectories.insert(directory, false);
                                }
                            }
                        }
                    }

                    if (remove) {
                        removeTrackFromDatabase(db, id);

                        files.removeAt(i);
                        --i;
                        --max;

                        ids.removeAt(i);
                        modificationTimeHash.remove(id);
                        mediaArtHash.remove(id);
                    }
                }

                // Remove deleted media art
                {
                    auto i = mediaArtHash.begin();
                    const auto end = mediaArtHash.end();
                    while (i != end) {
                        const QString& mediaArt = i.value();
                        if (!mediaArt.isEmpty() && !QFile::exists(mediaArt)) {
                            QSqlQuery query(db);
                            query.prepare(QStringLiteral("UPDATE tracks SET mediaArt = '' WHERE id = ?"));
                            query.addBindValue(i.key());
                            query.exec();

                            i = mediaArtHash.erase(i);
                        } else {
                            ++i;
                        }
                    }
                }

                QHash<QByteArray, QString> embeddedMediaArtHash;
                QDir mediaArtDir(mMediaArtDirectory);
                {
                    const QList<QFileInfo> files(mediaArtDir.entryInfoList(QDir::Files));
                    for (const QFileInfo& info : files) {
                        const QString baseName(info.baseName());
                        const int index = baseName.indexOf(QLatin1String("-embedded"));
                        if (index != -1) {
                            embeddedMediaArtHash.insert(baseName.left(index).toLatin1(), info.filePath());
                        }
                    }
                }

                QHash<QString, QString> mediaArtDirectoriesHash;
                const QMimeDatabase mimeDb;

                const bool useDirectoryMediaArt = Settings::instance()->useDirectoryMediaArt();

                for (const QString& directory : libraryDirectories) {
                    QDirIterator iterator(directory, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
                    while (iterator.hasNext()) {
                        if (!qApp) {
                            qWarning() << "app shutdown, stop updating";
                            db.commit();
                            return;
                        }

                        const QString filePath(iterator.next());
                        const QFileInfo fileInfo(filePath);

                        if (fileInfo.isDir()) {
                            continue;
                        }

                        if (!fileInfo.isReadable()) {
                            continue;
                        }

                        {
                            const QString directoryPath(fileInfo.path());
                            if (noMediaDirectories.contains(directoryPath)) {
                                if (noMediaDirectories.value(directoryPath)) {
                                    continue;
                                }
                            } else {
                                if (QFileInfo(QDir(directoryPath).filePath(QLatin1String(".nomedia"))).isFile()) {
                                    noMediaDirectories.insert(directoryPath, true);
                                    continue;
                                } else {
                                    noMediaDirectories.insert(directoryPath, false);
                                }
                            }
                        }

                        const int fileIndex = files.indexOf(filePath);

                        if (fileIndex == -1) {
                            QString mimeType(mimeDb.mimeTypeForFile(filePath, QMimeDatabase::MatchExtension).name());
                            if (mimeTypesByExtension.contains(mimeType)) {
                                mimeType = mimeDb.mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name();
                                if (mimeTypesByContent.contains(mimeType)) {
                                    int id;
                                    if (ids.isEmpty()) {
                                        id = 0;
                                    } else {
                                        id = ids.last() + 1;
                                    }
                                    ids.append(id);
                                    updateTrackInDatabase(db,
                                                          false,
                                                          id,
                                                          fileInfo,
                                                          tagutils::getTrackInfo(fileInfo, mimeType),
                                                          getTrackMediaArt(tagutils::getTrackInfo(fileInfo, mimeType),
                                                                           embeddedMediaArtHash,
                                                                           fileInfo,
                                                                           mediaArtDirectoriesHash,
                                                                           useDirectoryMediaArt));
                                }
                            }
                        } else {
                            const int id = ids.at(files.indexOf(filePath));

                            const long long modificationTime = fileInfo.lastModified().toMSecsSinceEpoch();
                            if (modificationTime == modificationTimeHash.value(id)) {
                                const QString mediaArt(mediaArtHash.value(id));
                                if (!mediaArt.startsWith(mMediaArtDirectory) ||
                                        (QFileInfo(mediaArt).fileName().contains(QLatin1String("-embedded")) && useDirectoryMediaArt)) {
                                    const QString newMediaArt(getTrackMediaArt(tagutils::getTrackInfo(fileInfo, mimeDb.mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name()),
                                                                               embeddedMediaArtHash,
                                                                               fileInfo,
                                                                               mediaArtDirectoriesHash,
                                                                               useDirectoryMediaArt));
                                    if (!newMediaArt.isEmpty() && newMediaArt != mediaArt) {
                                        QSqlQuery query(db);
                                        query.prepare(QStringLiteral("UPDATE tracks SET mediaArt = ? WHERE id = ?"));
                                        query.addBindValue(newMediaArt);
                                        query.addBindValue(id);
                                        query.exec();
                                    }
                                }
                            } else {
                                const QString mimeType(mimeDb.mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name());
                                if (mimeTypesByContent.contains(mimeType)) {
                                    ids.append(ids.last() + 1);
                                    updateTrackInDatabase(db,
                                                          true,
                                                          ids.last(),
                                                          fileInfo,
                                                          tagutils::getTrackInfo(fileInfo, mimeType),
                                                          getTrackMediaArt(tagutils::getTrackInfo(fileInfo, mimeType),
                                                                           embeddedMediaArtHash,
                                                                           fileInfo,
                                                                           mediaArtDirectoriesHash,
                                                                           useDirectoryMediaArt));
                                } else {
                                    removeTrackFromDatabase(db, id);
                                }
                            }
                        }
                    }
                }

                QVector<QString> allMediaArt;
                {
                    QSqlQuery query(QLatin1String("SELECT DISTINCT(mediaArt) FROM tracks WHERE mediaArt != ''"), db);
                    if (query.lastError().type() == QSqlError::NoError) {
                        while (query.next()) {
                            allMediaArt.append(query.value(0).toString());
                        }
                    }
                }

                {
                    const QList<QFileInfo> files(mediaArtDir.entryInfoList(QDir::Files));
                    for (const QFileInfo& info : files) {
                        if (!allMediaArt.contains(info.filePath())) {
                            if (!QFile::remove(info.filePath())) {
                                qWarning() << "failed to remove file:" << info.filePath();
                            }
                        }
                    }
                }

                db.commit();
            }
            QSqlDatabase::removeDatabase(rescanConnectionName);
            qDebug() << "end scanning files";
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

    QString LibraryUtils::getTrackMediaArt(const tagutils::Info& info,
                                           QHash<QByteArray, QString>& embeddedMediaArtHash,
                                           const QFileInfo& fileInfo,
                                           QHash<QString, QString>& directoriesHash,
                                           bool useDirectoriesMediaArt)
    {
        QString mediaArt;
        if (useDirectoriesMediaArt) {
            mediaArt = findMediaArtForDirectory(directoriesHash, fileInfo.path());
            if (mediaArt.isEmpty()) {
                if (!info.mediaArtData.isEmpty()) {
                    mediaArt = saveEmbeddedMediaArt(info.mediaArtData, embeddedMediaArtHash);
                }
            }
        } else {
            if (info.mediaArtData.isEmpty()) {
                mediaArt = findMediaArtForDirectory(directoriesHash, fileInfo.path());
            } else {
                mediaArt = saveEmbeddedMediaArt(info.mediaArtData, embeddedMediaArtHash);
            }
        }
        return mediaArt;
    }

    QString LibraryUtils::saveEmbeddedMediaArt(const QByteArray& data, QHash<QByteArray, QString>& embeddedMediaArtHash)
    {
        const QByteArray md5(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex());
        if (embeddedMediaArtHash.contains(md5)) {
            return embeddedMediaArtHash.value(md5);
        }

        const QString suffix(mMimeDb.mimeTypeForData(data).preferredSuffix());
        if (suffix.isEmpty()) {
            return QString();
        }

        const QString filePath(QString::fromLatin1("%1/%2-embedded.%3")
                               .arg(mMediaArtDirectory, QString::fromLatin1(md5), suffix));
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(data);
            embeddedMediaArtHash.insert(md5, filePath);
            return filePath;
        }

        return QString();
    }
}
