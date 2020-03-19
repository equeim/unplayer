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
#include <QRunnable>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QThreadPool>
#include <QUuid>
#include <QtConcurrentRun>

#include "albumsmodel.h"
#include "settings.h"
#include "stdutils.h"
#include "tagutils.h"
#include "utilsfunctions.h"

namespace unplayer
{
    namespace
    {
        const QLatin1String rescanConnectionName("unplayer_rescan");
        const QLatin1String removeFilesConnectionName("unplayer_remove");
        const QLatin1String saveTagsConnectionName("unplayer_save");

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

        //const QLatin1String wvSuffix("wv");
        //const QLatin1String wvpSuffix("wvp");

        inline QString emptyIfNull(const QString& string)
        {
            if (string.isNull()) {
                return QLatin1String("");
            }
            return string;
        }

        template<typename Function>
        void forEachOrOnce(const QStringList& strings, const Function& exec)
        {
            if (strings.empty()) {
                exec(QLatin1String(""));
            } else {
                for (const QString& string : strings) {
                    exec(string);
                }
            }
        }

        template<bool SetMediaArt = true>
        void addTrackToDatabase(const QSqlDatabase& db,
                                int id,
                                const QString& filePath,
                                long long modificationTime,
                                tagutils::Info& info,
                                const QString& directoryMediaArt = QString(),
                                const QString& embeddedMediaArt = QString())
            {
                if (info.albumArtists.isEmpty() && !info.artists.isEmpty()) {
                    info.albumArtists = info.artists;
                } else if (info.artists.isEmpty() && !info.albumArtists.isEmpty()) {
                    info.artists = info.albumArtists;
                }

                QSqlQuery query(db);
                query.prepare([&]() {
                    QString queryString(SetMediaArt ? QStringLiteral("INSERT INTO tracks (id, modificationTime, year, trackNumber, duration, filePath, title, artist, albumArtist, album, discNumber, genre, directoryMediaArt, embeddedMediaArt) "
                                                                     "VALUES ")
                                                    : QStringLiteral("INSERT INTO tracks (id, modificationTime, year, trackNumber, duration, filePath, title, artist, albumArtist, album, discNumber, genre) "
                                                                     "VALUES "));
                    const auto sizeOrOne = [](const QStringList& strings) {
                        return strings.empty() ? 1 : strings.size();
                    };
                    const int count = sizeOrOne(info.artists) * sizeOrOne(info.albums) * sizeOrOne(info.genres);
                    for (int i = 0; i < count; ++i) {
                        queryString.push_back(SetMediaArt ? QStringLiteral("(%1, %2, %3, %4, %5, ?, ?, ?, ?, ?, ?, ?, ?, ?)")
                                                          : QStringLiteral("(%1, %2, %3, %4, %5, ?, ?, ?, ?, ?, ?, ?)"));
                        if (i != (count - 1)) {
                            queryString.push_back(QLatin1Char(','));
                        }
                    }
                    queryString = queryString.arg(id).arg(modificationTime).arg(info.year).arg(info.trackNumber).arg(info.duration);
                    return queryString;
                }());

                forEachOrOnce(info.artists, [&](const QString& artist) {
                    forEachOrOnce(info.albumArtists, [&](const QString& albumArtist) {
                        forEachOrOnce(info.albums, [&](const QString& album) {
                            forEachOrOnce(info.genres, [&](const QString& genre) {
                                query.addBindValue(filePath);
                                query.addBindValue(info.title);
                                query.addBindValue(emptyIfNull(artist));
                                query.addBindValue(emptyIfNull(albumArtist));
                                query.addBindValue(emptyIfNull(album));
                                query.addBindValue(emptyIfNull(info.discNumber));
                                query.addBindValue(emptyIfNull(genre));
                                if (SetMediaArt) {
                                    query.addBindValue(emptyIfNull(directoryMediaArt));
                                    query.addBindValue(emptyIfNull(embeddedMediaArt));
                                }
                            });
                        });
                    });
                });

                if (!query.exec()) {
                    qWarning() << "failed to insert track in the database" << query.lastError();
                }
            }

        class LibraryUpdateRunnableNotifier : public QObject
        {
            Q_OBJECT
        signals:
            void stageChanged(unplayer::LibraryUtils::UpdateStage newStage);
            void foundFilesChanged(int found);
            void extractedFilesChanged(int extracted);
            void finished();
        };

        class LibraryUpdateRunnable : public QRunnable
        {
        public:
            explicit LibraryUpdateRunnable(const QString& mediaArtDirectory)
                : mMediaArtDirectory(mediaArtDirectory),
                  mCancel(false)
            {

            }

            LibraryUpdateRunnableNotifier* notifier()
            {
                return &mNotifier;
            }

            void cancel()
            {
                qInfo("Cancel updating database");
                mCancel = true;
            }

            void run() override
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

                struct FileToAdd
                {
                    QString filePath;
                    QString directoryMediaArt;
                    Extension extension;
                    int id;
                };
                std::vector<FileToAdd> filesToAdd;

                std::unordered_map<QByteArray, QString> embeddedMediaArtFiles;
                {
                    const QFileInfoList files(QDir(mMediaArtDirectory).entryInfoList({QLatin1String("*-embedded.*")}, QDir::Files));
                    embeddedMediaArtFiles.reserve(static_cast<size_t>(files.size()));
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
                            if (mCancel) {
                                return;
                            }

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
                    emit mNotifier.stageChanged(LibraryUtils::ScanningStage);

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

                                filesToAdd.push_back({std::move(filePath), directoryMediaArt, extension, ++lastId});
                                ++foundFiles;
                                emit mNotifier.foundFilesChanged(foundFiles);
                            } else {
                                // File is in database

                                const FileInDb& file = foundInDb->second;

                                const long long modificationTime = getLastModifiedTime(filePath);
                                if (modificationTime == file.modificationTime) {
                                    // File has not changed
                                    if (file.embeddedMediaArtDeleted) {
                                        const QString embeddedMediaArt(saveEmbeddedMediaArt(tagutils::getTrackInfo(filePath, extension, mimeDb).mediaArtData,
                                                                                            embeddedMediaArtFiles,
                                                                                            mimeDb));
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
                                    filesToAdd.push_back({foundInDb->first, directoryMediaArt, extension, file.id});
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
                    emit mNotifier.stageChanged(LibraryUtils::ExtractingStage);

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

                            addTrackToDatabase(db,
                                               file.id,
                                               file.filePath,
                                               getLastModifiedTime(file.filePath),
                                               trackInfo,
                                               file.directoryMediaArt,
                                               saveEmbeddedMediaArt(trackInfo.mediaArtData,
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

                emit mNotifier.stageChanged(LibraryUtils::FinishingStage);

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

                qInfo("End updating database (last stage took %.3f s)", static_cast<double>(stageTimer.elapsed()) / 1000.0);
                qInfo("Total time: %.3f s", static_cast<double>(timer.elapsed()) / 1000.0);
            }

            QString saveEmbeddedMediaArt(const QByteArray& data, std::unordered_map<QByteArray, QString>& embeddedMediaArtFiles, const QMimeDatabase& mimeDb)
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
                    embeddedMediaArtFiles.insert({std::move(md5), filePath});
                    return filePath;
                }

                return QString();
            }
        private:
            LibraryUpdateRunnableNotifier mNotifier;
            QString mMediaArtDirectory;
            std::atomic_bool mCancel;
        };

        const QString& databasePath()
        {
            static const QString path(QString::fromLatin1("%1/library.sqlite").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation)));
            return path;
        }
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

    DatabaseGuard::~DatabaseGuard()
    {
        QSqlDatabase::removeDatabase(connectionName);
    }

    CommitGuard::~CommitGuard()
    {
        db.commit();
    }

    const QString LibraryUtils::databaseType(QLatin1String("QSQLITE"));
    const size_t LibraryUtils::maxDbVariableCount = 999; // SQLITE_MAX_VARIABLE_NUMBER

    Extension LibraryUtils::extensionFromSuffix(const QString& suffix)
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

            {ogaSuffix, Extension::OGG},
            {oggSuffix, Extension::OGG},
            {opusSuffix, Extension::OPUS},

            {apeSuffix, Extension::APE},

            {mkaSuffix, Extension::MKA},

            {wavSuffix, Extension::WAV},

            //{wvSuffix, Extension::WAVPACK},
            //{wvpSuffix, Extension::WAVPACK}
        };
        static const auto end(extensions.end());

        const auto found(extensions.find(suffix.toLower()));
        if (found == end) {
            return Extension::Other;
        }
        return found->second;
    }

    bool LibraryUtils::isExtensionSupported(const QString& suffix)
    {
        return extensionFromSuffix(suffix) != Extension::Other;
    }

    bool LibraryUtils::isVideoExtensionSupported(const QString& suffix)
    {
        static const std::unordered_set<QString> videoMimeTypesExtensions{
            QLatin1String("mp4"),
            QLatin1String("m4v"),
            QLatin1String("f4v"),
            QLatin1String("lrv"),

            /*QLatin1String("mpeg"),
            QLatin1String("mpg"),
            QLatin1String("mp2"),
            QLatin1String("mpe"),
            QLatin1String("vob"),*/

            QLatin1String("mkv"),

            QLatin1String("ogv")
        };
        return contains(videoMimeTypesExtensions, suffix.toLower());
    }

    QSqlDatabase LibraryUtils::openDatabase(const QString& connectionName)
    {
        auto db(QSqlDatabase::addDatabase(databaseType, connectionName));
        db.setDatabaseName(databasePath());
        if (!db.open()) {
            qWarning() << "failed to open database:" << db.lastError();
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
        qDebug() << "init db";

        const QString dataDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
        if (!QDir().mkpath(dataDir)) {
            qWarning() << "failed to create data directory";
            return;
        }

        QSqlDatabase db(openDatabase());
        if (!db.isOpen()) {
            return;
        }

        bool createTable = !db.tables().contains(QLatin1String("tracks"));
        if (!createTable) {
            static const std::vector<QLatin1String> fields{QLatin1String("id"),
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
            qInfo("Creating table");
            QSqlQuery query(QLatin1String("CREATE TABLE tracks ("
                                          "    id INTEGER,"
                                          "    filePath TEXT,"
                                          "    modificationTime INTEGER,"
                                          "    title TEXT COLLATE NOCASE,"
                                          "    artist TEXT COLLATE NOCASE,"
                                          "    albumArtist TEXT COLLATE NOCASE,"
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
        if (mLibraryUpdateRunnable || !mDatabaseInitialized) {
            return;
        }

        auto runnable = new LibraryUpdateRunnable(mMediaArtDirectory);
        QObject::connect(runnable->notifier(), &LibraryUpdateRunnableNotifier::stageChanged, this, [this](UpdateStage newStage) {
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
            mLibraryUpdateStage = NoneStage;
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
        mLibraryUpdateStage = PreparingStage;
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
        QSqlQuery query(QLatin1String("DELETE from tracks"));
        if (query.lastError().type() != QSqlError::NoError) {
            qWarning() << "failed to reset database";
        }
        if (!QDir(mMediaArtDirectory).removeRecursively()) {
            qWarning() << "failed to remove media art directory";
        }
        emit databaseChanged();
    }

    bool LibraryUtils::isDatabaseInitialized() const
    {
        return mDatabaseInitialized;
    }

    bool LibraryUtils::isCreatedTable() const
    {
        return mCreatedTable;
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

    void LibraryUtils::removeArtists(std::vector<QString>&& artists, bool deleteFiles)
    {
        if (mRemovingFiles || !mDatabaseInitialized) {
            return;
        }

        mRemovingFiles = true;
        emit removingFilesChanged();

        auto future = QtConcurrent::run([deleteFiles, artists = std::move(artists)] {
            const DatabaseGuard databaseGuard{removeFilesConnectionName};

            // Open database
            QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
            if (!db.isOpen()) {
                return;
            }

            db.transaction();
            const CommitGuard commitGuard{db};

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
            const DatabaseGuard databaseGuard{removeFilesConnectionName};

            // Open database
            QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
            if (!db.isOpen()) {
                return;
            }

            db.transaction();
            const CommitGuard commitGuard{db};

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
            const DatabaseGuard databaseGuard{removeFilesConnectionName};

            // Open database
            QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
            if (!db.isOpen()) {
                return;
            }

            db.transaction();
            const CommitGuard commitGuard{db};

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
            const DatabaseGuard databaseGuard{removeFilesConnectionName};

            QElapsedTimer timer;
            timer.start();

            // Open database
            QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
            if (!db.isOpen()) {
                return;
            }

            qInfo() << "db opening time:" << timer.restart();

            db.transaction();
            const CommitGuard commitGuard{db};

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

        auto future = QtConcurrent::run([files, tags, incrementTrackNumber]() {
            qInfo("Start saving tags");

            QElapsedTimer timer;
            timer.start();

            QMimeDatabase mimeDb;
            std::vector<tagutils::Info> infos(incrementTrackNumber ? tagutils::saveTags<true>(files, tags, mimeDb)
                                                                   : tagutils::saveTags<false>(files, tags, mimeDb));
            if (!qApp) {
                return;
            }

            const DatabaseGuard databaseGuard{saveTagsConnectionName};

            // Open database
            QSqlDatabase db(LibraryUtils::openDatabase(databaseGuard.connectionName));
            if (!db.isOpen()) {
                return;
            }

            db.transaction();
            const CommitGuard commitGuard{db};

            batchedCount(infos.size(), LibraryUtils::maxDbVariableCount, [&](size_t first, size_t count) {
                if (!qApp) {
                    return;
                }

                QString queryString(QLatin1String("DELETE FROM tracks WHERE "));
                queryString.push_back(QLatin1String("filePath = ?"));
                for (size_t i = first + 1, max = first + count; i < max; ++i) {
                    queryString.push_back(QLatin1String(" OR filePath = ?"));
                }

                QSqlQuery query(db);
                query.prepare(queryString);
                for (size_t i = first, max = first + count; i < max; ++i) {
                    query.addBindValue(emptyIfNull(infos[static_cast<std::vector<tagutils::Info>::size_type>(i)].filePath));
                }
                if (!query.exec()) {
                    qWarning() << "failed to remove tracks:" << query.lastError();
                }
            });

            int lastId = -1;
            {
                QSqlQuery query(QLatin1String("SELECT MAX(id) FROM tracks"), db);
                if (query.lastError().type() != QSqlError::NoError) {
                    qWarning() << "failed to get last id:" << query.lastError();
                    return;
                }
                if (query.next()) {
                    lastId = query.value(0).toInt();
                }
            }

            for (tagutils::Info& info : infos) {
                if (info.fileTypeValid) {
                    if (info.title.isEmpty()) {
                        info.title = QFileInfo(info.filePath).fileName();
                    }
                    addTrackToDatabase<false>(db, ++lastId, info.filePath, getLastModifiedTime(info.filePath), info);
                }
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

    LibraryUtils::LibraryUtils(QObject* parent)
        : QObject(parent),
          mDatabaseInitialized(false),
          mCreatedTable(false),
          mLibraryUpdateRunnable(nullptr),
          mLibraryUpdateStage(NoneStage),
          mFoundTracks(0),
          mExtractedTracks(0),
          mRemovingFiles(false),
          mSavingTags(false),
          mMediaArtDirectory(QString::fromLatin1("%1/media-art").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)))
    {
        qRegisterMetaType<UpdateStage>();
        initDatabase();
        QObject::connect(this, &LibraryUtils::databaseChanged, this, &LibraryUtils::mediaArtChanged);
    }
}

#include "libraryutils.moc"
