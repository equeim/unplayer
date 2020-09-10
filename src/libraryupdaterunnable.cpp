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
                long long modificationTime;
            };

            struct TracksInDbResult
            {
                std::unordered_map<QString, TrackInDb> tracksInDb;
                std::unordered_map<QString, QString> mediaArtDirectoriesInDbHash;
            };

            TracksInDbResult getTracksFromDatabase(std::vector<int>& tracksToRemove, std::unordered_map<QString, bool>& noMediaDirectories);

            struct TrackToAdd
            {
                QString filePath;
                QString directoryMediaArt;
                fileutils::Extension extension;
            };

            std::vector<TrackToAdd> scanFilesystem(TracksInDbResult& tracksInDbResult,
                                                   std::vector<int>& tracksToRemove,
                                                   std::unordered_map<QString, bool>& noMediaDirectories,
                                                   std::unordered_map<QByteArray, QString>& embeddedMediaArtFiles);

            int addTracks(const std::vector<TrackToAdd>& tracksToAdd,
                          std::unordered_map<QByteArray, QString>& embeddedMediaArtFiles,
                          const QElapsedTimer& stageTimer);

            bool isBlacklisted(const QString& path);

            std::atomic_bool& mCancel;

            QStringList mLibraryDirectories;
            QStringList mBlacklistedDirectories;
            DatabaseConnectionGuard mDatabaseGuard{QLatin1String("unplayer_update")};
            QSqlDatabase& mDb{mDatabaseGuard.db};
            QSqlQuery mQuery{mDb};
            QMimeDatabase mMimeDb;

        signals:
            void stageChanged(unplayer::LibraryUtils::UpdateStage newStage);
            void foundFilesChanged(int found);
            void extractedFilesChanged(int extracted);
            void finished();
        };

        LibraryUpdater::LibraryUpdater(std::atomic_bool& cancelFlag)
            : mCancel(cancelFlag)
        {

        }

        void LibraryUpdater::update()
        {
            const auto finishedGuard(qScopeGuard([&] {
                emit finished();
            }));

            if (mCancel) {
                return;
            }

            qInfo("Start updating database");
            QElapsedTimer timer;
            timer.start();
            QElapsedTimer stageTimer;
            stageTimer.start();

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
                    // Library directories
                    mLibraryDirectories = prepareLibraryDirectories(Settings::instance()->libraryDirectories());
                    mBlacklistedDirectories = prepareLibraryDirectories(Settings::instance()->blacklistedDirectories());

                    std::vector<int> tracksToRemove;
                    {
                        std::unordered_map<QString, bool> noMediaDirectories;
                        TracksInDbResult trackInDbResult(getTracksFromDatabase(tracksToRemove, noMediaDirectories));
                        auto& tracksInDb = trackInDbResult.tracksInDb;

                        if (mCancel) {
                            return;
                        }

                        qInfo("Tracks in database: %zd (took %.3f s)", tracksInDb.size(), static_cast<double>(stageTimer.restart()) / 1000.0);
                        qInfo("Tracks to remove: %zd", tracksToRemove.size());

                        qInfo("Start scanning filesystem");
                        emit stageChanged(LibraryUtils::ScanningStage);

                        tracksToAdd = scanFilesystem(trackInDbResult,
                                                     tracksToRemove,
                                                     noMediaDirectories,
                                                     embeddedMediaArtFiles);

                        if (mCancel) {
                            return;
                        }

                        qInfo("End scanning filesystem (took %.3f s), need to extract tags from %zu files", static_cast<double>(stageTimer.restart()) / 1000.0, tracksToAdd.size());
                    }

                    if (!tracksToRemove.empty()) {
                        if (LibraryUtils::removeTracksFromDbByIds(tracksToRemove, mDb, mCancel)) {
                            qInfo("Removed %zu tracks from database (took %.3f s)", tracksToRemove.size(), static_cast<double>(stageTimer.restart()) / 1000.0);
                        }
                    }
                }

                if (mCancel) {
                    return;
                }

                if (!tracksToAdd.empty()) {
                    qInfo("Start extracting tags from files");
                    emit stageChanged(LibraryUtils::ExtractingStage);
                    const int count = addTracks(tracksToAdd, embeddedMediaArtFiles, stageTimer);
                    qInfo("Added %d tracks to database (took %.3f s)", count, static_cast<double>(stageTimer.restart()) / 1000.0);
                }
            }

            if (mCancel) {
                return;
            }

            emit stageChanged(LibraryUtils::FinishingStage);

            LibraryUtils::removeUnusedCategories(mDb);
            LibraryUtils::removeUnusedMediaArt(mDb, mCancel);

            qInfo("End updating database (last stage took %.3f s)", static_cast<double>(stageTimer.elapsed()) / 1000.0);
            qInfo("Total time: %.3f s", static_cast<double>(timer.elapsed()) / 1000.0);
        }

        LibraryUpdater::TracksInDbResult LibraryUpdater::getTracksFromDatabase(std::vector<int>& tracksToRemove, std::unordered_map<QString, bool>& noMediaDirectories)
        {
            std::unordered_map<QString, TrackInDb> tracksInDb;
            std::unordered_map<QString, QString> mediaArtDirectoriesInDbHash;

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

            if (!mQuery.exec(QLatin1String("SELECT id, filePath, modificationTime, directoryMediaArt, embeddedMediaArt FROM tracks ORDER BY id"))) {
                qWarning() << "failed to get files from database" << mQuery.lastError();
                mCancel = true;
                return {};
            }

            while (mQuery.next()) {
                if (mCancel) {
                    return {};
                }

                const int id(mQuery.value(0).toInt());
                QString filePath(mQuery.value(1).toString());
                const QFileInfo fileInfo(filePath);
                QString directory(fileInfo.path());

                bool remove = false;
                if (!fileInfo.exists() || fileInfo.isDir() || !fileInfo.isReadable()) {
                    remove = true;
                } else {
                    remove = true;
                    for (const QString& dir : mLibraryDirectories) {
                        if (filePath.startsWith(dir)) {
                            remove = false;
                            break;
                        }
                    }
                    if (!remove) {
                        remove = isBlacklisted(filePath);
                    }
                    if (!remove) {
                        remove = isNoMediaDirectory(directory, noMediaDirectories);
                    }
                }

                if (remove) {
                    tracksToRemove.push_back(id);
                } else {
                    tracksInDb.emplace(std::move(filePath), TrackInDb{id,
                                                                      !checkExistance(mQuery.value(4).toString()),
                                                                      mQuery.value(2).toLongLong()});
                    mediaArtDirectoriesInDbHash.insert({std::move(directory), mQuery.value(3).toString()});
                }
            }

            return {std::move(tracksInDb), std::move(mediaArtDirectoriesInDbHash)};
        }

        std::vector<LibraryUpdater::TrackToAdd> LibraryUpdater::scanFilesystem(LibraryUpdater::TracksInDbResult& tracksInDbResult,
                                                                               std::vector<int>& tracksToRemove,
                                                                               std::unordered_map<QString, bool>& noMediaDirectories,
                                                                               std::unordered_map<QByteArray, QString>& embeddedMediaArtFiles)
        {
            std::vector<TrackToAdd> tracksToAdd;

            auto& tracksInDb = tracksInDbResult.tracksInDb;
            const auto tracksInDbEnd(tracksInDb.end());
            auto& mediaArtDirectoriesInDbHash = tracksInDbResult.mediaArtDirectoriesInDbHash;
            const auto mediaArtDirectoriesInDbHashEnd(mediaArtDirectoriesInDbHash.end());

            std::unordered_map<QString, QString> mediaArtDirectoriesHash;

            QString directory;
            QString directoryMediaArt;

            for (const QString& topLevelDirectory : mLibraryDirectories) {
                if (mCancel) {
                    return tracksToAdd;
                }

                QDirIterator iterator(topLevelDirectory, QDir::Files | QDir::Readable, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
                while (iterator.hasNext()) {
                    if (mCancel) {
                        return tracksToAdd;
                    }

                    QString filePath(iterator.next());
                    const QFileInfo fileInfo(iterator.fileInfo());

                    const fileutils::Extension extension = fileutils::extensionFromSuffix(fileInfo.suffix());
                    if (extension == fileutils::Extension::Other) {
                        continue;
                    }

                    if (fileInfo.path() != directory) {
                        directory = fileInfo.path();
                        directoryMediaArt = MediaArtUtils::findMediaArtForDirectory(mediaArtDirectoriesHash, directory, mCancel);

                        const auto directoryMediaArtInDb(mediaArtDirectoriesInDbHash.find(directory));
                        if (directoryMediaArtInDb != mediaArtDirectoriesInDbHashEnd) {
                            if (directoryMediaArtInDb->second != directoryMediaArt) {
                                mQuery.prepare(QStringLiteral("UPDATE tracks SET directoryMediaArt = ? WHERE instr(filePath, ?) = 1"));
                                mQuery.addBindValue(nullIfEmpty(directoryMediaArt));
                                mQuery.addBindValue(QString(directory % QLatin1Char('/')));
                                if (!mQuery.exec()) {
                                    qWarning() << mQuery.lastError();
                                }
                            }
                            mediaArtDirectoriesInDbHash.erase(directoryMediaArtInDb);
                        }
                    }

                    const auto foundInDb(tracksInDb.find(filePath));
                    if (foundInDb == tracksInDbEnd) {
                        // File is not in database

                        if (isNoMediaDirectory(fileInfo.path(), noMediaDirectories)) {
                            continue;
                        }

                        if (isBlacklisted(filePath)) {
                            continue;
                        }

                        tracksToAdd.push_back({std::move(filePath), directoryMediaArt, extension});
                        emit foundFilesChanged(static_cast<int>(tracksToAdd.size()));
                    } else {
                        // File is in database

                        const TrackInDb& file = foundInDb->second;

                        const long long modificationTime = getLastModifiedTime(filePath);
                        if (modificationTime == file.modificationTime) {
                            // File has not changed
                            if (file.embeddedMediaArtDeleted) {
                                const QString embeddedMediaArt(MediaArtUtils::saveEmbeddedMediaArt(tagutils::getTrackInfo(filePath, extension, mMimeDb).mediaArtData,
                                                                                                   embeddedMediaArtFiles,
                                                                                                   mMimeDb));
                                mQuery.prepare(QStringLiteral("UPDATE tracks SET embeddedMediaArt = ? WHERE id = ?"));
                                mQuery.addBindValue(nullIfEmpty(embeddedMediaArt));
                                mQuery.addBindValue(file.id);
                                if (!mQuery.exec()) {
                                    qWarning() << mQuery.lastError();
                                }
                            }
                        } else {
                            // File has changed
                            tracksToRemove.push_back(file.id);
                            tracksToAdd.push_back({foundInDb->first, directoryMediaArt, extension});
                            emit foundFilesChanged(static_cast<int>(tracksToAdd.size()));
                        }
                    }
                }
            }

            return tracksToAdd;
        }

        int LibraryUpdater::addTracks(const std::vector<LibraryUpdater::TrackToAdd>& tracksToAdd,
                                      std::unordered_map<QByteArray, QString>& embeddedMediaArtFiles,
                                      const QElapsedTimer& stageTimer)
        {
            int count = 0;

            LibraryTracksAdder adder(mDb);
            for (const TrackToAdd& track : tracksToAdd) {
                if (mCancel) {
                    return count;
                }

                tagutils::Info trackInfo(tagutils::getTrackInfo(track.filePath, track.extension, mMimeDb));
                if (trackInfo.fileTypeValid) {
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
                        qInfo("Extracted tags from %d of %zu files (%.3f s elapsed)", count, tracksToAdd.size(), static_cast<double>(stageTimer.elapsed()) / 1000.0);
                    }
                    emit extractedFilesChanged(count);
                }
            }

            return count;
        }

        bool LibraryUpdater::isBlacklisted(const QString& path)
        {
            for (const QString& directory : mBlacklistedDirectories) {
                if (path.startsWith(directory)) {
                    return true;
                }
            }
            return false;
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
        QObject::connect(&updater, &LibraryUpdater::finished, this, &LibraryUpdateRunnable::finished);
        updater.update();
    }
}

#include "libraryupdaterunnable.moc"
