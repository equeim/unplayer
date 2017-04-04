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

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
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
        const QLatin1String mp4MimeType("audio/mp4");
        const QLatin1String mp4bMimeType("audio/x-m4b");
        const QLatin1String mpegMimeType("audio/mpeg");
        const QLatin1String oggMimeType("audio/ogg");
        const QLatin1String vorbisOggMimeType("audio/x-vorbis+ogg");
        const QLatin1String flacOggMimeType("audio/x-flac+ogg");
        const QLatin1String opusOggMimeType("audio/x-opus+ogg");
        const QLatin1String apeMimeType("audio/x-ape");
        const QLatin1String matroskaMimeType("audio/x-matroska");
        const QLatin1String wavMimeType("audio/x-wav");
        const QLatin1String wavpackMimeType("audio/x-wavpack");

        LibraryUtils* instancePointer = nullptr;
        bool databaseInitialized = false;
        bool createdTable = false;
        bool updating = false;

        QString mediaArtDirectory()
        {
            return QString::fromLatin1("%1/media-art").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
        }

        void removeTrackFromDatabase(const QSqlDatabase& db, const QString& filePath)
        {
            QSqlQuery query(db);
            query.prepare(QStringLiteral("DELETE FROM tracks WHERE filePath = ?"));
            query.addBindValue(filePath);
            if (!query.exec()) {
                qWarning() << "failed to remove file from database" << query.lastQuery();
            }
        }

        void updateTrackInDatabase(const QSqlDatabase& db, bool inDb, const QFileInfo& fileInfo, const tagutils::Info& info, const QString& mediaArt)
        {
            if (inDb) {
                removeTrackFromDatabase(db, fileInfo.filePath());
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
                        query.prepare(QStringLiteral("INSERT INTO tracks (filePath, modificationTime, title, artist, album, year, trackNumber, genre, duration, mediaArt) "
                                                     "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
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
        if (string == oggMimeType) {
            return MimeType::Ogg;
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
        if (string == matroskaMimeType) {
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

    const QVector<QString> LibraryUtils::supportedMimeTypes{flacMimeType,
                                                            mp4MimeType,
                                                            mp4bMimeType,
                                                            mpegMimeType,
                                                            oggMimeType,
                                                            vorbisOggMimeType,
                                                            flacOggMimeType,
                                                            opusOggMimeType,
                                                            apeMimeType,
                                                            matroskaMimeType,
                                                            wavMimeType,
                                                            wavpackMimeType};

    LibraryUtils* LibraryUtils::instance()
    {
        if (!instancePointer) {
            instancePointer = new LibraryUtils(qApp);
        }
        return instancePointer;
    }

    QString LibraryUtils::databaseFilePath()
    {
        return QString::fromLatin1("%1/library.sqlite").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
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
        db.setDatabaseName(databaseFilePath());
        if (!db.open()) {
            qWarning() << "failed to open database" << db.lastError();
            return;
        }

        if (db.tables().isEmpty()) {
            QSqlQuery query(QLatin1String("CREATE TABLE tracks ("
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
                qWarning() << "failed to create table" << query.lastError();
                return;
            }

            createdTable = true;
        }

        databaseInitialized = true;
    }

    void LibraryUtils::updateDatabase()
    {
        if (updating) {
            return;
        }

        updating = true;
        emit updatingChanged();

        auto future = QtConcurrent::run([]() {
            qDebug() << "start scanning files";
            {
                auto db = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"), rescanConnectionName);
                db.setDatabaseName(databaseFilePath());
                if (!db.open()) {
                    QSqlDatabase::removeDatabase(rescanConnectionName);
                    qWarning() << "failed to open database" << db.lastError();
                    return;
                }

                db.transaction();


                QStringList files;
                QHash<QString, long long> modificationTimeHash;
                QHash<QString, QString> mediaArtHash;
                {
                    QSqlQuery query(QLatin1String("SELECT filePath, modificationTime, mediaArt FROM tracks"), db);
                    if (query.lastError().type() != QSqlError::NoError) {
                        qWarning() << "failed to get files from database" << query.lastError();
                        return;
                    }
                    while (query.next()) {
                        const QString filePath(query.value(0).toString());
                        files.append(filePath);
                        modificationTimeHash.insert(filePath, query.value(1).toLongLong());
                        mediaArtHash.insert(filePath, query.value(2).toString());
                    }
                }

                QStringList libraryDirectories(Settings::instance()->libraryDirectories());
                libraryDirectories.removeDuplicates();

                const QString mediaArtDirectoryPath(mediaArtDirectory());

                for (int i = 0, max = files.size(); i < max; ++i) {
                    const QString filePath(files.at(i));
                    bool remove = false;
                    if (QFile::exists(filePath)) {
                        remove = true;
                        for (const QString& directory : libraryDirectories) {
                            if (filePath.startsWith(directory)) {
                                remove = false;
                                break;
                            }
                        }
                    } else {
                        remove = true;
                    }

                    if (remove) {
                        removeTrackFromDatabase(db, filePath);

                        files.removeAt(i);
                        --i;
                        --max;
                        modificationTimeHash.remove(filePath);
                        mediaArtHash.remove(filePath);
                    }
                }

                {
                    auto i = mediaArtHash.begin();
                    const auto end = mediaArtHash.end();
                    while (i != end) {
                        const QString& mediaArt = i.value();
                        if (!mediaArt.isEmpty() && !QFile::exists(mediaArt)) {
                            i = mediaArtHash.erase(i);
                        } else {
                            ++i;
                        }
                    }
                }

                QHash<QString, QString> mediaArtDirectoriesHash;
                const QMimeDatabase mimeDb;

                for (const QString& directory : libraryDirectories) {
                    QDirIterator iterator(directory, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
                    while (iterator.hasNext()) {
                        if (!qApp) {
                            db.commit();
                            qWarning() << "app shut down";
                            return;
                        }

                        const QString& filePath(iterator.next());
                        const QFileInfo fileInfo(filePath);

                        if (fileInfo.isDir()) {
                            continue;
                        }

                        if (!fileInfo.isReadable()) {
                            continue;
                        }

                        const bool inDb = files.contains(filePath);

                        if (inDb) {
                            const long long modificationTime = fileInfo.lastModified().toMSecsSinceEpoch();
                            if (modificationTime == modificationTimeHash.value(filePath)) {
                                const QString mediaArt(mediaArtHash.value(filePath));
                                if (!mediaArt.startsWith(mediaArtDirectoryPath)) {
                                    const QString newMediaArt(findMediaArtForDirectory(mediaArtDirectoriesHash, fileInfo.path()));
                                    if (newMediaArt != mediaArt) {
                                        QSqlQuery query;
                                        query.prepare(QStringLiteral("UPDATE tracks SET mediaArt = ? WHERE filePath = ?"));
                                        query.addBindValue(newMediaArt);
                                        query.addBindValue(filePath);
                                        query.exec();
                                    }
                                }
                            } else {
                                const QString mimeType(mimeDb.mimeTypeForFile(filePath, QMimeDatabase::MatchDefault).name());
                                if (supportedMimeTypes.contains(mimeType)) {
                                    updateTrackInDatabase(db,
                                                          true,
                                                          fileInfo,
                                                          tagutils::getTrackInfo(fileInfo, mimeType),
                                                          findMediaArtForDirectory(mediaArtDirectoriesHash, fileInfo.path()));
                                } else {
                                    removeTrackFromDatabase(db, filePath);
                                }
                            }
                        } else {
                            QString mimeType(mimeDb.mimeTypeForFile(filePath, QMimeDatabase::MatchExtension).name());
                            if (supportedMimeTypes.contains(mimeType)) {
                                mimeType = mimeDb.mimeTypeForFile(filePath, QMimeDatabase::MatchDefault).name();
                                if (supportedMimeTypes.contains(mimeType)) {
                                    updateTrackInDatabase(db,
                                                          false,
                                                          fileInfo,
                                                          tagutils::getTrackInfo(fileInfo, mimeType),
                                                          findMediaArtForDirectory(mediaArtDirectoriesHash, fileInfo.path()));
                                }
                            }
                        }
                    }
                }

                QVector<QString> allMediaArt;
                {
                    QSqlQuery query(QLatin1String("SELECT DISTINCT(mediaArt) FROM tracks"));
                    if (query.lastError().type() == QSqlError::NoError) {
                        while (query.next()) {
                            allMediaArt.append(query.value(0).toString());
                        }
                    }
                }
                QDir mediaArtDir(mediaArtDirectoryPath);
                for (const QFileInfo& info : mediaArtDir.entryInfoList(QDir::Files)) {
                    if (!allMediaArt.contains(info.filePath())) {
                        if (!QFile::remove(info.filePath())) {
                            qWarning() << "failed to remove file:" << info.filePath();
                        }
                    }
                }

                db.commit();
            }
            QSqlDatabase::removeDatabase(rescanConnectionName);
            qDebug() << "end scanning files";
        });

        auto watcher = new QFutureWatcher<void>(this);
        QObject::connect(watcher, &QFutureWatcher<void>::finished, this, [=]() {
            updating = false;
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
        if (!QDir(mediaArtDirectory()).removeRecursively()) {
            qWarning() << "failed to remove media art directory";
        }
        emit databaseChanged();
    }

    bool LibraryUtils::isDatabaseInitialized()
    {
        return databaseInitialized;
    }

    bool LibraryUtils::isCreatedTable()
    {
        return createdTable;
    }

    bool LibraryUtils::isUpdating()
    {
        return updating;
    }

    int LibraryUtils::artistsCount()
    {
        if (!databaseInitialized) {
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
        if (!databaseInitialized) {
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
        if (!databaseInitialized) {
            return 0;
        }

        QSqlQuery query(QLatin1String("SELECT COUNT(DISTINCT(filePath)) FROM tracks"));
        if (query.next()) {
            return query.value(0).toInt();
        }
        return 0;
    }

    int LibraryUtils::tracksDuration()
    {
        if (!databaseInitialized) {
            return 0;
        }

        QSqlQuery query(QLatin1String("SELECT SUM(duration) FROM (SELECT duration from tracks GROUP BY filePath)"));
        if (query.next()) {
            return query.value(0).toInt();
        }
        return 0;
    }

    QString LibraryUtils::randomMediaArt()
    {
        if (!databaseInitialized) {
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
        if (!databaseInitialized) {
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
        if (!databaseInitialized) {
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
        const QString mediaArtDirPath(mediaArtDirectory());
        if (!QDir().mkpath(mediaArtDirPath)) {
            qWarning() << "failed to create media art directory:" << mediaArtDirPath;
            return;
        }

        const QString newFilePath(QString::fromLatin1("%1/%2.%3")
                                  .arg(mediaArtDirPath)
                                  .arg(QUuid::createUuid().toString())
                                  .arg(QFileInfo(mediaArt).suffix()));

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

    LibraryUtils::LibraryUtils(QObject* parent)
        : QObject(parent)
    {
        initDatabase();
        QObject::connect(this, &LibraryUtils::databaseChanged, this, &LibraryUtils::mediaArtChanged);
    }
}
