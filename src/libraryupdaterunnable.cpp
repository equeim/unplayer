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

#include <algorithm>

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QStringBuilder>
#include <QSqlError>

#include "librarytracksadder.h"
#include "libraryutils.h"
#include "mediaartutils.h"
#include "qscopeguard.h"
#include "settings.h"
#include "sqlutils.h"
#include "tagutils.h"
#include "utilsfunctions.h"

namespace unplayer
{
    namespace
    {
        bool isNoMediaDirectory(const QString& directory, std::unordered_map<QString, bool>& noMediaDirectories)
        {
            {
                const auto found(noMediaDirectories.find(directory));
                if (found != noMediaDirectories.end()) {
                    return found->second;
                }
            }
            const bool noMedia = QFileInfo(directory % QStringLiteral("/.nomedia")).isFile();
            noMediaDirectories.insert({directory, noMedia});
            return noMedia;
        }

        const QStringList prepareLibraryDirectories(QStringList&& dirs) {
            if (!dirs.isEmpty()) {
                for (QString& dir : dirs) {
                    if (!dir.endsWith(QLatin1Char('/'))) {
                        dir.push_back(QLatin1Char('/'));
                    }
                }
                dirs.removeDuplicates();

                for (int i = dirs.size() - 1; i >= 0; --i) {
                    const QString& dir = dirs[i];
                    const auto found(std::find_if(dirs.begin(), dirs.end(), [&](const QString& d) {
                        return d != dir && dir.startsWith(d);
                    }));
                    if (found != dirs.end()) {
                        dirs.removeAt(i);
                    }
                }
            }
            return std::move(dirs);
        }

        class LibraryUpdater final : public QObject
        {
            Q_OBJECT
        public:
            explicit LibraryUpdater(std::atomic_bool& cancelFlag);

            void update();
        private:
            struct TrackInDb
            {
                int id;
                bool embeddedMediaArtDeleted;
                bool removeFromDatabase;
                bool checkedIfDirectoryMediaArtChanged;
                long long modificationTime;
            };

            struct TracksInDbResult
            {
                std::unordered_map<QString, TrackInDb> tracksInDb;
                /**
                 * @brief Map of directory paths to their media art which are in db
                 */
                std::unordered_map<QString, QString> mediaArtDirectoriesInDbHash;
            };

            /**
             * @brief Extracts information about existing tracks in db
             * @return `TracksInDbResult` instance
             */
            TracksInDbResult getTracksFromDatabase();

            struct TrackToAdd
            {
                QString filePath;
                QString directoryMediaArt;
                bool foundDirectoryMediaArt;
                fileutils::Extension extension;
            };

            struct ScanFilesystemResult
            {
                std::vector<TrackToAdd> tracksToAdd;
                size_t tracksToRemoveFromDatabaseCount;

                struct ChangedDirectoryMediaArt
                {
                    QString newDirectoryMediaArt;
                    /**
                     * @brief Ids of tracks which need their directory media art to be updated
                     */
                    std::vector<int> trackIds;
                };
                /**
                 * @brief Map of directory paths to `LibraryUpdater::ScanFilesystemResult::ChangedDirectoryMediaArt` instances
                 */
                std::unordered_map<QString, ChangedDirectoryMediaArt> changedDirectoriesMediaArt;
            };

            /**
             * @brief Iterates over library directories, finds new tracks, determines whether existing ones changed or removed
             * @param tracksInDbResult      Return value from `getTracksFromDatabase()`
             * @param embeddedMediaArtFiles Map of embedded media art files' MD5 hashes to their paths
             * @return `ScanFilesystemResult` instance
             */
            ScanFilesystemResult scanFilesystem(TracksInDbResult& tracksInDbResult,
                                                std::unordered_map<QByteArray, QString>& embeddedMediaArtFiles);

            /**
             * @brief Checks if this directory has media art in db and it has changed
             * @param directoryPath              Path to directory
             * @param newDirectoryMediaArt       New media art for this directory
             * @param directoriesMediaArtInDb    Map of directory paths to their media art which are in db
             * @param changedDirectoriesMediaArt Map of directory paths to `LibraryUpdater::ScanFilesystemResult::ChangedDirectoryMediaArt` instances
             * @return Pointer to vector of existing track ids for this directory,
             *         or nullptr if this directory media art has not changed or is not in db
             */
            static std::vector<int>* checkIfDirectoryMediaArtIsInDbAndChanged(const QString& directoryPath,
                                                                              const QString& newDirectoryMediaArt,
                                                                              std::unordered_map<QString, QString>& directoriesMediaArtInDb,
                                                                              std::unordered_map<QString, ScanFilesystemResult::ChangedDirectoryMediaArt>& changedDrectoriesMediaArt);

            bool isPathBlacklisted(const QString& path);

            /**
             * @brief Update tracks which directory media art was changed
             * @param changedDirectoriesMediaArt Map of directory paths to `LibraryUpdater::ScanFilesystemResult::ChangedDirectoryMediaArt` instances
             */
            void updateChangedDirectoriesMediaArt(std::unordered_map<QString, ScanFilesystemResult::ChangedDirectoryMediaArt> changedDirectoriesMediaArt);

            int addTracks(std::vector<TrackToAdd> tracksToAdd,
                          std::unordered_map<QByteArray, QString>& embeddedMediaArtFiles);

            std::atomic_bool& mCancel;

            QStringList mLibraryDirectories;
            QStringList mBlacklistedDirectories;
            DatabaseConnectionGuard mDatabaseGuard{QLatin1String("unplayer_update")};
            QSqlDatabase& mDb{mDatabaseGuard.db};
            QMimeDatabase mMimeDb;
            QElapsedTimer mStageTimer;

        signals:
            void stageChanged(unplayer::LibraryUtils::UpdateStage newStage);
            void foundFilesChanged(int found);
            void extractedFilesChanged(int extracted);
        };

        LibraryUpdater::LibraryUpdater(std::atomic_bool& cancelFlag)
            : mCancel(cancelFlag)
        {

        }

        void LibraryUpdater::update()
        {
            if (mCancel) {
                return;
            }

            qInfo("Start updating database");
            QElapsedTimer timer;
            timer.start();
            mStageTimer.start();

            // Open database

            if (!mDb.isOpen()) {
                return;
            }

            const TransactionGuard transactionGuard(mDb);

            // Create media art directory
            if (!QDir().mkpath(MediaArtUtils::mediaArtDirectory())) {
                qWarning() << "failed to create media art directory:" << MediaArtUtils::mediaArtDirectory();
            }

            if (!LibraryUtils::dropIndexes(mDb)) {
                qWarning("Failed to drop indexes");
            }

            const auto indexesGuard(qScopeGuard([&] {
                if (!LibraryUtils::createIndexes(mDb)) {
                    qWarning("Failed to create indexes");
                }
            }));

            {
                std::unordered_map<QByteArray, QString> embeddedMediaArtFiles(MediaArtUtils::getEmbeddedMediaArtFiles());
                std::vector<TrackToAdd> tracksToAdd;

                {
                    std::vector<int> tracksToRemove;
                    {
                        TracksInDbResult tracksInDbResult(getTracksFromDatabase());
                        auto& tracksInDb = tracksInDbResult.tracksInDb;

                        if (mCancel) {
                            return;
                        }

                        qInfo("Tracks in database: %zd (took %.3f s)", tracksInDb.size(), static_cast<double>(mStageTimer.restart()) / 1000.0);

                        qInfo("Start scanning filesystem");
                        emit stageChanged(LibraryUtils::ScanningStage);

                        // Library directories
                        mLibraryDirectories = prepareLibraryDirectories(Settings::instance()->libraryDirectories());
                        mBlacklistedDirectories = prepareLibraryDirectories(Settings::instance()->blacklistedDirectories());

                        ScanFilesystemResult scanFilesystemResult(scanFilesystem(tracksInDbResult, embeddedMediaArtFiles));
                        tracksToAdd = std::move(scanFilesystemResult.tracksToAdd);

                        if (mCancel) {
                            return;
                        }

                        if (scanFilesystemResult.tracksToRemoveFromDatabaseCount > 0) {
                            tracksToRemove.reserve(scanFilesystemResult.tracksToRemoveFromDatabaseCount);
                            for (const auto& i : tracksInDbResult.tracksInDb) {
                                const TrackInDb& track = i.second;
                                if (track.removeFromDatabase) {
                                    tracksToRemove.push_back(track.id);
                                }
                            }
                        }

                        qInfo("End scanning filesystem (took %.3f s), need to extract tags from %zu files", static_cast<double>(mStageTimer.restart()) / 1000.0, tracksToAdd.size());

                        updateChangedDirectoriesMediaArt(std::move(scanFilesystemResult.changedDirectoriesMediaArt));
                    }

                    if (!tracksToRemove.empty()) {
                        qInfo("Tracks to remove: %zd", tracksToRemove.size());
                        if (LibraryUtils::removeTracksFromDbByIds(tracksToRemove, mDb, mCancel)) {
                            qInfo("Removed %zu tracks from database (took %.3f s)", tracksToRemove.size(), static_cast<double>(mStageTimer.restart()) / 1000.0);
                        }
                        tracksToRemove.clear();
                    }
                }

                if (mCancel) {
                    return;
                }

                if (!tracksToAdd.empty()) {
                    qInfo("Start extracting tags from files");
                    emit stageChanged(LibraryUtils::ExtractingStage);
                    const int count = addTracks(std::move(tracksToAdd), embeddedMediaArtFiles);
                    qInfo("Added %d tracks to database (took %.3f s)", count, static_cast<double>(mStageTimer.restart()) / 1000.0);
                }
            }

            if (mCancel) {
                return;
            }

            emit stageChanged(LibraryUtils::FinishingStage);

            LibraryUtils::removeUnusedCategories(mDb);
            LibraryUtils::removeUnusedMediaArt(mDb, mCancel);

            qInfo("End updating database (last stage took %.3f s)", static_cast<double>(mStageTimer.elapsed()) / 1000.0);
            qInfo("Total time: %.3f s", static_cast<double>(timer.elapsed()) / 1000.0);
        }

        LibraryUpdater::TracksInDbResult LibraryUpdater::getTracksFromDatabase()
        {
            TracksInDbResult result{};
            std::unordered_map<QString, TrackInDb>& tracksInDb = result.tracksInDb;
            std::unordered_map<QString, QString>& mediaArtDirectoriesInDbHash = result.mediaArtDirectoriesInDbHash;

            // Extract tracks from database

            std::unordered_map<QString, bool> embeddedMediaArtExistanceHash;
            const auto checkExistanceOfEmbeddedMediaArt = [&](QString&& mediaArt) {
                if (mediaArt.isEmpty()) {
                    return true;
                }

                const auto found(embeddedMediaArtExistanceHash.find(mediaArt));
                if (found == embeddedMediaArtExistanceHash.end()) {
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

            enum
            {
                IdField,
                FilePathField,
                ModificationTimeField,
                DirectoryMediaArtField,
                EmbeddedMediaArtField
            };

            QSqlQuery query(mDb);
            if (!query.exec(QLatin1String("SELECT id, filePath, modificationTime, directoryMediaArt, embeddedMediaArt FROM tracks ORDER BY id"))) {
                qWarning() << "failed to get files from database" << query.lastError();
                mCancel = true;
                return {};
            }

            reserveFromQuery(tracksInDb, query);

            while (query.next()) {
                if (mCancel) {
                    return {};
                }

                const int id(query.value(IdField).toInt());
                QString filePath(query.value(FilePathField).toString());

                mediaArtDirectoriesInDbHash.insert({QFileInfo(filePath).path(), query.value(DirectoryMediaArtField).toString()});

                tracksInDb.emplace(std::move(filePath), TrackInDb{id,
                                                                  !checkExistanceOfEmbeddedMediaArt(query.value(EmbeddedMediaArtField).toString()),
                                                                  true,
                                                                  false,
                                                                  query.value(ModificationTimeField).toLongLong()});
            }

            return result;
        }

        LibraryUpdater::ScanFilesystemResult LibraryUpdater::scanFilesystem(LibraryUpdater::TracksInDbResult& tracksInDbResult,
                                                                            std::unordered_map<QByteArray, QString>& embeddedMediaArtFiles)
        {
            ScanFilesystemResult result{};
            std::vector<TrackToAdd>& tracksToAdd = result.tracksToAdd;
            auto& changedDirectoriesMediaArt = result.changedDirectoriesMediaArt;

            auto& tracksInDb = tracksInDbResult.tracksInDb;
            const auto tracksInDbEnd(tracksInDb.end());
            auto& directoriesMediaArtInDb = tracksInDbResult.mediaArtDirectoriesInDbHash;

            result.tracksToRemoveFromDatabaseCount = tracksInDb.size();

            std::unordered_map<QString, bool> noMediaDirectoriesCache;
            std::unordered_map<QString, QString> directoriesMediaArt;

            QString currentDirectory;
            QString currentDirectoryMediaArt;
            bool foundCurrentDirectoryMediaArt = false;
            std::vector<int>* currentDirectoryChangedMediaArtTrackIds = nullptr;

            const auto onFoundCurrentDirectoryMediaArt = [&](const QString& directoryMediaArt) {
                currentDirectoryMediaArt = directoryMediaArt;
                foundCurrentDirectoryMediaArt = true;
                currentDirectoryChangedMediaArtTrackIds = checkIfDirectoryMediaArtIsInDbAndChanged(currentDirectory, currentDirectoryMediaArt, directoriesMediaArtInDb, changedDirectoriesMediaArt);
            };

            for (const QString& topLevelDirectory : mLibraryDirectories) {
                if (mCancel) {
                    return result;
                }

                QDirIterator iterator(topLevelDirectory, QDir::Files | QDir::Readable, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
                while (iterator.hasNext()) {
                    if (mCancel) {
                        return result;
                    }

                    QString filePath(iterator.next());
                    const QFileInfo fileInfo(iterator.fileInfo());

                    {
                        const QString directoryPath(fileInfo.path());
                        if (directoryPath != currentDirectory) {
                            // Entered new directory
                            currentDirectory = directoryPath;

                            if (isNoMediaDirectory(currentDirectory, noMediaDirectoriesCache)) {
                                continue;
                            }
                            if (isPathBlacklisted(fileInfo.filePath())) {
                                continue;
                            }

                            currentDirectoryChangedMediaArtTrackIds = nullptr;

                            const auto foundMediaArt(directoriesMediaArt.find(currentDirectory));
                            if (foundMediaArt != directoriesMediaArt.end()) {
                                onFoundCurrentDirectoryMediaArt(foundMediaArt->second);
                            } else {
                                currentDirectoryMediaArt.clear();
                                foundCurrentDirectoryMediaArt = false;
                            }
                        }
                    }

                    const QString suffixLowered(fileInfo.suffix().toLower());
                    const fileutils::Extension extension = fileutils::extensionFromSuffixLowered(suffixLowered);
                    if (extension == fileutils::Extension::Other) {
                        if (!foundCurrentDirectoryMediaArt && MediaArtUtils::isMediaArtFileSuffixLowered(fileInfo, suffixLowered)) {
                            directoriesMediaArt.emplace(currentDirectory, filePath);
                            onFoundCurrentDirectoryMediaArt(filePath);
                        }
                        continue;
                    }

                    const auto foundInDb(tracksInDb.find(filePath));
                    if (foundInDb == tracksInDbEnd) {
                        // File is not in database
                        tracksToAdd.push_back({std::move(filePath), currentDirectoryMediaArt, foundCurrentDirectoryMediaArt, extension});
                        emit foundFilesChanged(static_cast<int>(tracksToAdd.size()));
                    } else {
                        // File is in database

                        TrackInDb& file = foundInDb->second;

                        const long long modificationTime = getLastModifiedTime(filePath);
                        if (modificationTime == file.modificationTime) {
                            // File has not changed
                            file.removeFromDatabase = false;
                            --result.tracksToRemoveFromDatabaseCount;

                            if (file.embeddedMediaArtDeleted) {
                                const QString embeddedMediaArt(MediaArtUtils::saveEmbeddedMediaArt(tagutils::getTrackInfo(filePath, extension, mMimeDb).mediaArtData,
                                                                                                   embeddedMediaArtFiles,
                                                                                                   mMimeDb));
                                QSqlQuery query(mDb);
                                query.prepare(QStringLiteral("UPDATE tracks SET embeddedMediaArt = ? WHERE id = ?"));
                                query.addBindValue(nullIfEmpty(embeddedMediaArt));
                                query.addBindValue(file.id);
                                if (!query.exec()) {
                                    qWarning() << query.lastError();
                                }
                            }

                            if (foundCurrentDirectoryMediaArt) {
                                file.checkedIfDirectoryMediaArtChanged = true;
                                if (currentDirectoryChangedMediaArtTrackIds) {
                                    currentDirectoryChangedMediaArtTrackIds->push_back(file.id);
                                }
                            }
                        } else {
                            // File has changed
                            tracksToAdd.push_back({std::move(filePath), currentDirectoryMediaArt, foundCurrentDirectoryMediaArt, extension});
                            emit foundFilesChanged(static_cast<int>(tracksToAdd.size()));
                        }
                    }
                }
            }

            // Process directory media art for tracks which didn't have it yet at the moment of iteration

            const auto directoriesMediaArtEnd(directoriesMediaArt.end());

            for (auto& i : tracksInDb) {
                TrackInDb& trackInDb = i.second;
                if (!trackInDb.removeFromDatabase && !trackInDb.checkedIfDirectoryMediaArtChanged) {
                    const QString directoryPath(QFileInfo(i.first).path());
                    std::vector<int>* trackIds = nullptr;
                    const auto foundMediaArt(directoriesMediaArt.find(directoryPath));
                    if (foundMediaArt != directoriesMediaArtEnd) {
                        trackIds = checkIfDirectoryMediaArtIsInDbAndChanged(directoryPath, foundMediaArt->second, directoriesMediaArtInDb, changedDirectoriesMediaArt);
                    } else {
                        trackIds = checkIfDirectoryMediaArtIsInDbAndChanged(directoryPath, QString(), directoriesMediaArtInDb, changedDirectoriesMediaArt);
                    }
                    if (trackIds) {
                        trackIds->push_back(trackInDb.id);
                    }
                    trackInDb.checkedIfDirectoryMediaArtChanged = true;
                }
            }

            for (TrackToAdd& trackToAdd : tracksToAdd) {
                if (!trackToAdd.foundDirectoryMediaArt) {
                    const auto foundMediaArt(directoriesMediaArt.find(QFileInfo(trackToAdd.filePath).path()));
                    if (foundMediaArt != directoriesMediaArtEnd) {
                        trackToAdd.directoryMediaArt = foundMediaArt->second;
                    }
                    trackToAdd.foundDirectoryMediaArt = true;
                }
            }

            return result;
        }

        std::vector<int>* LibraryUpdater::checkIfDirectoryMediaArtIsInDbAndChanged(const QString& directoryPath,
                                                                                   const QString& newDirectoryMediaArt,
                                                                                   std::unordered_map<QString, QString>& directoriesMediaArtInDb,
                                                                                   std::unordered_map<QString, ScanFilesystemResult::ChangedDirectoryMediaArt>& changedDirectoriesMediaArt)
        {
            const auto directoryMediaArtInDb(directoriesMediaArtInDb.find(directoryPath));
            if (directoryMediaArtInDb != directoriesMediaArtInDb.end()) {
                // This directory is in db and we haven't checked it yet
                if (directoryMediaArtInDb->second != newDirectoryMediaArt) {
                    // Media art has changed
                    const auto inserted(changedDirectoriesMediaArt.emplace(directoryPath, ScanFilesystemResult::ChangedDirectoryMediaArt{newDirectoryMediaArt, {}}));
                    if (inserted.second) {
                        return &inserted.first->second.trackIds;
                    }
                }
                // Remove it from db map
                directoriesMediaArtInDb.erase(directoryMediaArtInDb);
            } else {
                // This directory is not in db or we already checked it
                // See if its media art changed
                const auto changedDirectoryMediaArt(changedDirectoriesMediaArt.find(directoryPath));
                if (changedDirectoryMediaArt != changedDirectoriesMediaArt.end()) {
                    // Media art has changed
                    return &changedDirectoryMediaArt->second.trackIds;
                }
            }
            return nullptr;
        }

        bool LibraryUpdater::isPathBlacklisted(const QString& path)
        {
            for (const QString& directory : mBlacklistedDirectories) {
                if (path.startsWith(directory)) {
                    return true;
                }
            }
            return false;
        }

        void LibraryUpdater::updateChangedDirectoriesMediaArt(std::unordered_map<QString, LibraryUpdater::ScanFilesystemResult::ChangedDirectoryMediaArt> changedDirectoriesMediaArt)
        {
            if (!changedDirectoriesMediaArt.empty()) {
                qInfo("Update changed media art");
                QSqlQuery query(mDb);
                for (const auto& i : changedDirectoriesMediaArt) {
                    batchedCount(i.second.trackIds.size(), LibraryUtils::maxDbVariableCount, [&](size_t first, size_t count) {
                        query.prepare(QLatin1String("UPDATE tracks SET directoryMediaArt = ? WHERE id IN (") % makeInStringFromIds(i.second.trackIds, first, count));
                        query.addBindValue(nullIfEmpty(i.second.newDirectoryMediaArt));
                        if (!query.exec()) {
                            qWarning() << query.lastError();
                        }
                    });
                }
            }
        }

        int LibraryUpdater::addTracks(std::vector<LibraryUpdater::TrackToAdd> tracksToAdd,
                                      std::unordered_map<QByteArray, QString>& embeddedMediaArtFiles)
        {
            int count = 0;

            LibraryTracksAdder adder(mDb);
            for (TrackToAdd& track : tracksToAdd) {
                if (mCancel) {
                    return count;
                }

                tagutils::Info trackInfo(tagutils::getTrackInfo(track.filePath, track.extension, mMimeDb));
                if (trackInfo.fileTypeMatchesExtension && fileutils::isAudioCodecSupported(trackInfo.audioCodec)) {
                    ++count;

                    if (trackInfo.title.isEmpty()) {
                        trackInfo.title = QFileInfo(track.filePath).fileName();
                    }

                    adder.addTrackToDatabase(track.filePath,
                                             getLastModifiedTime(track.filePath),
                                             trackInfo,
                                             track.directoryMediaArt,
                                             MediaArtUtils::saveEmbeddedMediaArt(trackInfo.mediaArtData,
                                                                                 embeddedMediaArtFiles,
                                                                                 mMimeDb));
                    if ((count % 100) == 0) {
                        qInfo("Extracted tags from %d of %zu files (%.3f s elapsed)", count, tracksToAdd.size(), static_cast<double>(mStageTimer.elapsed()) / 1000.0);
                    }
                    emit extractedFilesChanged(count);
                }
            }

            return count;
        }
    }

    void LibraryUpdateRunnable::cancel()
    {
        qInfo("Cancel updating database");
        mCancelFlag = true;
    }

    void LibraryUpdateRunnable::run()
    {
        LibraryUpdater updater(mCancelFlag);
        QObject::connect(&updater, &LibraryUpdater::stageChanged, this, &LibraryUpdateRunnable::stageChanged);
        QObject::connect(&updater, &LibraryUpdater::foundFilesChanged, this, &LibraryUpdateRunnable::foundFilesChanged);
        QObject::connect(&updater, &LibraryUpdater::extractedFilesChanged, this, &LibraryUpdateRunnable::extractedFilesChanged);
        updater.update();
        emit finished();
    }
}

#include "libraryupdaterunnable.moc"
