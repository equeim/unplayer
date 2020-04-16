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

#ifndef UNPLAYER_LIBRARYUPDATERUNNABLE_H
#define UNPLAYER_LIBRARYUPDATERUNNABLE_H

#include <unordered_map>

#include <QMimeDatabase>
#include <QObject>
#include <QPair>
#include <QRunnable>
#include <QSqlQuery>
#include <QVector>

#include "fileutils.h"
#include "sqlutils.h"
#include "stdutils.h"


using QStringIntVectorPair = QPair<QString, QVector<int>>;
QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF(QStringIntVectorPair)

class QSqlDatabase;

namespace unplayer
{
    namespace tagutils
    {
        struct Info;
    }

    class LibraryTracksAdder
    {
    public:
        explicit LibraryTracksAdder(const QSqlDatabase& db);

        void addTrackToDatabase(const QString& filePath,
                                long long modificationTime,
                                tagutils::Info& info,
                                const QString& directoryMediaArt,
                                const QString& embeddedMediaArt);

    private:
        struct ArtistsOrGenres
        {
            explicit ArtistsOrGenres(QLatin1String table) : table(table) {}
            QLatin1String table;
            std::unordered_map<QString, int> ids{};
            int lastId = 0;
        };

        void getArtists();
        void getAlbums();
        void getGenres();

        int getArtistId(const QString& title);
        int getAlbumId(const QString& title, QVector<int>&& artistIds);
        int getGenreId(const QString& title);

        void addRelationship(int firstId, int secondId, const char* table);

        int addAlbum(const QString& title, QVector<int>&& artistIds);

        void getArtistsOrGenres(ArtistsOrGenres& ids);
        int getArtistOrGenreId(const QString& title, ArtistsOrGenres& ids);
        int addArtistOrGenre(const QString& title, ArtistsOrGenres& ids);

        int mLastTrackId;
        const QSqlDatabase& mDb;
        QSqlQuery mQuery{mDb};

        ArtistsOrGenres mArtists{QLatin1String("artists")};
        struct
        {
            std::unordered_map<QPair<QString, QVector<int>>, int> ids{};
            int lastId = 0;
        } mAlbums{};
        ArtistsOrGenres mGenres{QLatin1String("genres")};
    };

    class LibraryUpdateRunnableNotifier : public QObject
    {
        Q_OBJECT
    public:
        enum UpdateStage {
            NoneStage,
            PreparingStage,
            ScanningStage,
            ExtractingStage,
            FinishingStage
        };
        Q_ENUM(UpdateStage)
    signals:
        void stageChanged(unplayer::LibraryUpdateRunnableNotifier::UpdateStage newStage);
        void foundFilesChanged(int found);
        void extractedFilesChanged(int extracted);
        void finished();
    };

    class LibraryUpdateRunnable : public QRunnable
    {
    public:
        explicit LibraryUpdateRunnable(const QString& mediaArtDirectory);

        LibraryUpdateRunnableNotifier* notifier();

        void cancel();

        void run() override;
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

        static const QLatin1String databaseConnectionName;

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

        void removeTracks(const std::vector<int>& tracksToRemove);

        int addTracks(const std::vector<TrackToAdd>& tracksToAdd,
                      std::unordered_map<QByteArray, QString>& embeddedMediaArtFiles);

        void removeUnusedCategories();

        bool isBlacklisted(const QString& path);

        LibraryUpdateRunnableNotifier mNotifier;
        QString mMediaArtDirectory;
        std::atomic_bool mCancel;

        QStringList mLibraryDirectories;
        QStringList mBlacklistedDirectories;
        DatabaseGuard databaseGuard{databaseConnectionName};
        QSqlDatabase mDb;
        QSqlQuery mQuery;
        QMimeDatabase mMimeDb;
    };
}

#endif // UNPLAYER_LIBRARYUPDATERUNNABLE_H
