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

#include "mediaartutils.h"

#include <unordered_set>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStandardPaths>
#include <QThread>
#include <QUuid>

#include "fileutils.h"
#include "libraryutils.h"
#include "settings.h"
#include "sqlutils.h"
#include "stdutils.h"
#include "tagutils.h"

namespace unplayer
{
    class MediaArtUtilsWorker : public QObject
    {
        Q_OBJECT
    public:
        Q_INVOKABLE void getMediaArtForFile(const QString& filePath, const QString& albumForUserMediaArt, bool onlyExtractEmbedded)
        {
            if (!onlyExtractEmbedded) {
                openDb();
                if (!mDb.isOpen()) {
                    return;
                }

                bool foundInDb = false;

                mFileMediaArtQuery.addBindValue(filePath);
                if (mFileMediaArtQuery.exec()) {
                    QString foundUserMediaArt;
                    while (mFileMediaArtQuery.next()) {
                        foundInDb = true;
                        QString userMediaArt(mFileMediaArtQuery.value(UserMediaArtField).toString());
                        if (!userMediaArt.isEmpty()) {
                            if (albumForUserMediaArt.isEmpty()) {
                                foundUserMediaArt = std::move(userMediaArt);
                                break;
                            }
                            const QString album(mFileMediaArtQuery.value(AlbumField).toString());
                            if (album == albumForUserMediaArt) {
                                foundUserMediaArt = std::move(userMediaArt);
                                break;
                            }
                        }
                    }
                    if (foundInDb) {
                        mFileMediaArtQuery.last();
                        const QString mediaArt(mediaArtFromQuery(mFileMediaArtQuery, foundUserMediaArt));
                        emit gotMediaArtForFile(filePath, mediaArt, {}, {});
                    }
                    mFileMediaArtQuery.last();
                } else {
                    qWarning() << mFileMediaArtQuery.lastError();
                }
                mFileMediaArtQuery.finish();

                if (foundInDb) {
                    return;
                }
            }

            const QFileInfo fileInfo(filePath);
            const tagutils::Info info(tagutils::getTrackInfo(filePath,
                                                             fileutils::extensionFromSuffix(fileInfo.suffix()),
                                                             QMimeDatabase()));
            QString directoryMediaArt;
            if (!onlyExtractEmbedded) {
                std::unordered_map<QString, QString> mediaArtHash;
                directoryMediaArt = MediaArtUtils::findMediaArtForDirectory(mediaArtHash, fileInfo.path());
            }
            emit gotMediaArtForFile(filePath, {}, directoryMediaArt, info.mediaArtData);
        }

        Q_INVOKABLE void getRandomMediaArt(uintptr_t requestId)
        {
            openDb();
            if (!mDb.isOpen()) {
                return;
            }

            execQuery(mRandomMediaArtQuery, {}, [&](QString&& mediaArt) {
                emit gotRandomMediaArt(requestId, mediaArt);
            });
        }

        Q_INVOKABLE void getRandomMediaArtForArtist(int artistId)
        {
            openDb();
            if (!mDb.isOpen()) {
                return;
            }

            const auto emitSignal = [&](QString&& mediaArt) {
                emit gotRandomMediaArtForArtist(artistId, mediaArt);
            };

            if (artistId > 0) {
                execQuery(mArtistQuery, {artistId}, emitSignal);
            } else {
                execQuery(mNullArtistQuery, {}, emitSignal);
            }
        }

        Q_INVOKABLE void getRandomMediaArtForAlbum(int albumId)
        {
            openDb();
            if (!mDb.isOpen()) {
                return;
            }

            const auto emitSignal = [&](QString&& mediaArt) {
                emit gotRandomMediaArtForAlbum(albumId, mediaArt);
            };

            if (albumId > 0) {
                execQuery(mAlbumQuery, {albumId}, emitSignal);
            } else {
                execQuery(mNullAlbumQuery, {}, emitSignal);
            }
        }

        Q_INVOKABLE void getRandomMediaArtForArtistAndAlbum(int artistId, int albumId)
        {
            openDb();
            if (!mDb.isOpen()) {
                return;
            }

            const auto emitSignal = [&](QString&& mediaArt) {
                emit gotRandomMediaArtForArtistAndAlbum(artistId, albumId, mediaArt);
            };

            if (artistId > 0 && albumId > 0) {
                execQuery(mArtistAlbumQuery, {artistId, albumId}, emitSignal);
            } else if (artistId > 0) {
                execQuery(mArtistNullAlbumQuery, {artistId}, emitSignal);
            } else if (albumId > 0) {
                execQuery(mNullArtistAlbumQuery, {albumId}, emitSignal);
            } else {
                execQuery(mNullArtistNullAlbumQuery, {}, emitSignal);
            }
        }

        Q_INVOKABLE void getRandomMediaArtForGenre(int genreId)
        {
            openDb();
            if (!mDb.isOpen()) {
                return;
            }

            execQuery(mGenreQuery, {genreId}, [&](QString&& mediaArt) {
                emit gotRandomMediaArtForGenre(genreId, mediaArt);
            });
        }

    private:
        enum
        {
            DirectoryMediaArtField,
            EmbeddedMediaArtField,
            UserMediaArtField,

            AlbumField
        };

        void openDb()
        {
            if (!mOpenedDb) {
                mOpenedDb = true;
                mDb = LibraryUtils::openDatabase(mDatabaseGuard.connectionName);
                if (!mDb.isOpen()) {
                    return;
                }

                {
                    mFileMediaArtQuery = QSqlQuery(mDb);
                    if (!mFileMediaArtQuery.prepare(QLatin1String("SELECT directoryMediaArt, embeddedMediaArt, albums.userMediaArt, albums.title FROM tracks "
                                                                  "LEFT JOIN tracks_albums ON tracks_albums.trackId = tracks.id "
                                                                  "LEFT JOIN albums ON albums.id = tracks_albums.albumId "
                                                                  "WHERE filePath = ?"))) {
                        qWarning() << mFileMediaArtQuery.lastError();
                    }
                }

                const QString query(QLatin1String("SELECT * FROM ("
                                                    "SELECT DISTINCT directoryMediaArt, embeddedMediaArt, albums.userMediaArt FROM tracks "
                                                    "LEFT JOIN tracks_albums ON tracks_albums.trackId = tracks.id "
                                                    "LEFT JOIN albums ON albums.id = tracks_albums.albumId "
                                                    "%1 "
                                                    "WHERE (directoryMediaArt IS NOT NULL OR embeddedMediaArt IS NOT NULL OR albums.userMediaArt IS NOT NULL) %2"
                                                  ")"
                                                  "ORDER BY RANDOM() "
                                                  "LIMIT 1"));

                {
                    mRandomMediaArtQuery = QSqlQuery(mDb);
                    if (!mRandomMediaArtQuery.prepare(query.arg(QString(), QString()))) {
                        qWarning() << mRandomMediaArtQuery.lastError();
                    }
                }

                {
                    const QLatin1String join("LEFT JOIN tracks_artists ON tracks_artists.trackId = tracks.id ");
                    mArtistQuery = QSqlQuery(mDb);
                    if (!mArtistQuery.prepare(query.arg(join, QLatin1String("AND artistId = ?")))) {
                        qWarning() << mArtistQuery.lastError();
                    }
                    mNullArtistQuery = QSqlQuery(mDb);
                    if (!mNullArtistQuery.prepare(query.arg(join, QLatin1String("AND artistId IS NULL")))) {
                        qWarning() << mNullArtistQuery.lastError();
                    }
                }

                {
                    mAlbumQuery = QSqlQuery(mDb);
                    if (!mAlbumQuery.prepare(query.arg(QString(), "AND albumId = ?"))) {
                        qWarning() << mAlbumQuery.lastError();
                    }
                    mNullAlbumQuery = QSqlQuery(mDb);
                    if (!mNullAlbumQuery.prepare(query.arg(QString(), "AND albumId IS NULL"))) {
                        qWarning() << mNullAlbumQuery.lastError();
                    }
                }

                {
                    const QLatin1String join("LEFT JOIN tracks_artists ON tracks_artists.trackId = tracks.id ");
                    mArtistAlbumQuery = QSqlQuery(mDb);
                    if (!mArtistAlbumQuery.prepare(query.arg(join, QLatin1String("AND artistId = ? AND albumId = ? ")))) {
                        qWarning() << mArtistAlbumQuery.lastError();
                    }

                    mArtistNullAlbumQuery = QSqlQuery(mDb);
                    if (!mArtistNullAlbumQuery.prepare(query.arg(join, QLatin1String("AND artistId = ? AND albumId IS NULL ")))) {
                        qWarning() << mArtistNullAlbumQuery.lastError();
                    }

                    mNullArtistAlbumQuery = QSqlQuery(mDb);
                    if (!mNullArtistAlbumQuery.prepare(query.arg(join, QLatin1String("AND artistId IS NULL AND albumId = ? ")))) {
                        qWarning() << mNullArtistAlbumQuery.lastError();
                    }

                    mNullArtistNullAlbumQuery = QSqlQuery(mDb);
                    if (!mNullArtistNullAlbumQuery.prepare(query.arg(join, QLatin1String("AND artistId IS NULL AND albumId IS NULL ")))) {
                        qWarning() << mNullArtistNullAlbumQuery.lastError();
                    }
                }

                {
                    mGenreQuery = QSqlQuery(mDb);
                    if (!mGenreQuery.prepare(query.arg(QLatin1String("JOIN tracks_genres ON tracks_genres.trackId = tracks.id "),
                                                       QLatin1String("AND genreId = ? ")))) {
                        qWarning() << mGenreQuery.lastError();
                    }
                }
            }
        }

        static QString mediaArtFromQuery(const QSqlQuery& query)
        {
            return mediaArtFromQuery(query, query.value(UserMediaArtField).toString());
        }

        static QString mediaArtFromQuery(const QSqlQuery& query, const QString& userMediaArt)
        {
            if (!userMediaArt.isEmpty()) {
                return userMediaArt;
            }

            const QString directoryMediaArt(query.value(DirectoryMediaArtField).toString());
            const QString embeddedMediaArt(query.value(EmbeddedMediaArtField).toString());

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

        template<typename Func>
        void execQuery(QSqlQuery& query, QVariantList&& bindValues, const Func& emitSignal)
        {
            if (!bindValues.isEmpty()) {
                for (const QVariant& value : bindValues) {
                    query.addBindValue(value);
                }
            }
            if (query.exec()) {
                if (query.next()) {
                    emitSignal(mediaArtFromQuery(query));
                }
            } else {
                qWarning() << query.lastError();
            }
            query.finish();
        }

        DatabaseConnectionGuard mDatabaseGuard{QLatin1String("MediaArtProviderWorker")};
        QSqlDatabase mDb;
        bool mOpenedDb = false;

        QSqlQuery mFileMediaArtQuery;

        QSqlQuery mRandomMediaArtQuery;

        QSqlQuery mArtistQuery;
        QSqlQuery mNullArtistQuery;

        QSqlQuery mAlbumQuery;
        QSqlQuery mNullAlbumQuery;

        QSqlQuery mArtistAlbumQuery;
        QSqlQuery mArtistNullAlbumQuery;
        QSqlQuery mNullArtistAlbumQuery;
        QSqlQuery mNullArtistNullAlbumQuery;

        QSqlQuery mGenreQuery;

    signals:
        void gotMediaArtForFile(const QString& filePath, const QString& libraryMediaArt, const QString& directoryMediaArt, const QByteArray& embeddedMediaArtData);
        void gotRandomMediaArt(uintptr_t requestId, const QString& mediaArt);
        void gotRandomMediaArtForArtist(int artistId, const QString& mediaArt);
        void gotRandomMediaArtForAlbum(int albumId, const QString& mediaArt);
        void gotRandomMediaArtForArtistAndAlbum(int artistId, int albumId, const QString& mediaArt);
        void gotRandomMediaArtForGenre(int genreId, const QString& mediaArt);
    };

    namespace
    {
        MediaArtUtils* pInstance = nullptr;
    }

    MediaArtUtils* MediaArtUtils::instance()
    {
        if (!pInstance) {
            pInstance = new MediaArtUtils(qApp);
        }
        return pInstance;
    }

    void MediaArtUtils::deleteInstance()
    {
        if (pInstance) {
            delete pInstance;
            pInstance = nullptr;
        }
    }

    const QString& MediaArtUtils::mediaArtDirectory()
    {
        static const QString directory(QString::fromLatin1("%1/media-art").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)));
        return directory;
    }

    QString MediaArtUtils::findMediaArtForDirectory(std::unordered_map<QString, QString>& mediaArtHash, const QString& directoryPath, const std::atomic_bool& cancelFlag)
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

    std::unordered_map<QByteArray, QString> MediaArtUtils::getEmbeddedMediaArtFiles()
    {
        std::unordered_map<QByteArray, QString> embeddedMediaArtFiles;
        const QFileInfoList files(QDir(mediaArtDirectory()).entryInfoList({QStringLiteral("*-embedded.*")}, QDir::Files));
        embeddedMediaArtFiles.reserve(static_cast<size_t>(files.size()));
        for (const QFileInfo& info : files) {
            const QString baseName(info.baseName());
            const int index = baseName.indexOf(QStringLiteral("-embedded"));
            embeddedMediaArtFiles.emplace(baseName.left(index).toLatin1(), info.filePath());
        }
        return embeddedMediaArtFiles;
    }

    QString MediaArtUtils::saveEmbeddedMediaArt(const QByteArray& data, std::unordered_map<QByteArray, QString>& embeddedMediaArtFiles, const QMimeDatabase& mimeDb)
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
                               .arg(mediaArtDirectory(), QString::fromLatin1(md5), suffix));
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(data);
            embeddedMediaArtFiles.emplace(std::move(md5), filePath);
            return filePath;
        }

        return QString();
    }

    void MediaArtUtils::setUserMediaArt(int albumId, const QString& mediaArt)
    {
        if (!LibraryUtils::instance()->isDatabaseInitialized()) {
            return;
        }

        if (!QDir().mkpath(mediaArtDirectory())) {
            qWarning() << "Failed to create media art directory:" << mediaArtDirectory();
            return;
        }

        QString id(QUuid::createUuid().toString());
        id.remove(0, 1);
        id.chop(1);

        const QString newFilePath(QString::fromLatin1("%1/%2.%3")
                                  .arg(mediaArtDirectory(), id, QFileInfo(mediaArt).suffix()));

        if (!QFile::copy(mediaArt, newFilePath)) {
            qWarning() << "Failed to copy file from" << mediaArt << "to" << newFilePath;
            return;
        }

        QSqlQuery query;
        if (query.prepare(QLatin1String("UPDATE albums SET userMediaArt = ? WHERE id = ?"))) {
            query.addBindValue(newFilePath);
            query.addBindValue(albumId);
            if (query.exec()) {
                emit mediaArtChanged();
            } else {
                qWarning() << "Failed to update media art in the database:" << query.lastError();
            }
        } else {
            qWarning() << query.lastError();
        }
    }

    void MediaArtUtils::getMediaArtForFile(const QString& filePath, const QString& albumForUserMediaArt, bool onlyExtractEmbedded)
    {
        createWorker();
        mWorker->metaObject()->invokeMethod(mWorker, "getMediaArtForFile", Q_ARG(QString, filePath), Q_ARG(QString, albumForUserMediaArt), Q_ARG(bool, onlyExtractEmbedded));
    }

    void MediaArtUtils::getRandomMediaArt(uintptr_t requestId)
    {
        createWorker();
        mWorker->metaObject()->invokeMethod(mWorker, "getRandomMediaArt", Q_ARG(uintptr_t, requestId));
    }

    void MediaArtUtils::getRandomMediaArtForArtist(int artistId)
    {
        createWorker();
        mWorker->metaObject()->invokeMethod(mWorker, "getRandomMediaArtForArtist", Q_ARG(int, artistId));
    }

    void MediaArtUtils::getRandomMediaArtForAlbum(int albumId)
    {
        createWorker();
        mWorker->metaObject()->invokeMethod(mWorker, "getRandomMediaArtForAlbum", Q_ARG(int, albumId));
    }

    void MediaArtUtils::getRandomMediaArtForArtistAndAlbum(int artistId, int albumId)
    {
        createWorker();
        mWorker->metaObject()->invokeMethod(mWorker, "getRandomMediaArtForArtistAndAlbum", Q_ARG(int, artistId), Q_ARG(int, albumId));
    }

    void MediaArtUtils::getRandomMediaArtForGenre(int genreId)
    {
        createWorker();
        mWorker->metaObject()->invokeMethod(mWorker, "getRandomMediaArtForGenre", Qt::QueuedConnection, Q_ARG(int, genreId));
    }

    MediaArtUtils::MediaArtUtils(QObject* parent)
        : QObject(parent)
    {
        QObject::connect(LibraryUtils::instance(), &LibraryUtils::databaseChanged, this, &MediaArtUtils::mediaArtChanged);
    }

    MediaArtUtils::~MediaArtUtils()
    {
        if (mWorkerThread && mWorkerThread->isRunning()) {
            mWorkerThread->quit();
            mWorkerThread->wait();
        }
    }

    void MediaArtUtils::createWorker()
    {
        if (!mWorker) {
            qRegisterMetaType<uintptr_t>("uintptr_t");

            mWorkerThread = new QThread(this);

            mWorker = new MediaArtUtilsWorker();
            mWorker->moveToThread(mWorkerThread);
            QObject::connect(mWorkerThread, &QThread::finished, mWorker, &QObject::deleteLater);

            QObject::connect(mWorker, &MediaArtUtilsWorker::gotMediaArtForFile, this, &MediaArtUtils::gotMediaArtForFile);
            QObject::connect(mWorker, &MediaArtUtilsWorker::gotRandomMediaArt, this, &MediaArtUtils::gotRandomMediaArt);
            QObject::connect(mWorker, &MediaArtUtilsWorker::gotRandomMediaArtForArtist, this, &MediaArtUtils::gotRandomMediaArtForArtist);
            QObject::connect(mWorker, &MediaArtUtilsWorker::gotRandomMediaArtForAlbum, this, &MediaArtUtils::gotRandomMediaArtForAlbum);
            QObject::connect(mWorker, &MediaArtUtilsWorker::gotRandomMediaArtForArtistAndAlbum, this, &MediaArtUtils::gotRandomMediaArtForArtistAndAlbum);
            QObject::connect(mWorker, &MediaArtUtilsWorker::gotRandomMediaArtForGenre, this, &MediaArtUtils::gotRandomMediaArtForGenre);

            mWorkerThread->start();
        }
    }

    RandomMediaArt::RandomMediaArt()
    {
        QObject::connect(MediaArtUtils::instance(), &MediaArtUtils::gotRandomMediaArt, this, [this](uintptr_t requestId, const QString& mediaArt) {
            if (requestId == reinterpret_cast<uintptr_t>(this)) {
                mMediaArt = mediaArt;
                emit mediaArtChanged(mMediaArt);
            }
        });

        QObject::connect(LibraryUtils::instance(), &LibraryUtils::mediaArtChanged, this, [this] {
            mMediaArt.clear();
            mMediaArtRequested = false;
            emit mediaArtChanged(mMediaArt);
        });
    }

    const QString& RandomMediaArt::mediaArt()
    {
        if (!mMediaArtRequested) {
            MediaArtUtils::instance()->getRandomMediaArt(reinterpret_cast<uintptr_t>(this));
            mMediaArtRequested = true;
        }
        return mMediaArt;
    }

}

#include "mediaartutils.moc"
