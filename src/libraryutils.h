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

#ifndef UNPLAYER_LIBRARYUTILS_H
#define UNPLAYER_LIBRARYUTILS_H

#include <QObject>
#include <QSqlDatabase>

#include <atomic>
#include <unordered_map>
#include <unordered_set>

#include "libraryupdaterunnable.h"

class QMimeDatabase;
class QSqlQuery;

namespace unplayer
{
    struct Album;
    struct LibraryTrack;

    namespace tagutils
    {
        struct Info;
    }

    QString mediaArtFromQuery(const QSqlQuery& query, int directoryMediaArtField, int embeddedMediaArtField, const QString& userMediaArt);
    QString mediaArtFromQuery(const QSqlQuery& query, int directoryMediaArtField, int embeddedMediaArtField, int userMediaArtField);

    class LibraryUtils final : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(bool databaseInitialized READ isDatabaseInitialized CONSTANT)
        Q_PROPERTY(bool createdTables READ isCreatedTables CONSTANT)

        Q_PROPERTY(int artistsCount READ artistsCount NOTIFY databaseChanged)
        Q_PROPERTY(int albumsCount READ albumsCount NOTIFY databaseChanged)
        Q_PROPERTY(int tracksCount READ tracksCount NOTIFY databaseChanged)
        Q_PROPERTY(int tracksDuration READ tracksDuration NOTIFY databaseChanged)
        Q_PROPERTY(QString randomMediaArt READ randomMediaArt NOTIFY mediaArtChanged)

        Q_PROPERTY(bool updating READ isUpdating NOTIFY updatingChanged)
        Q_PROPERTY(unplayer::LibraryUpdateRunnable::UpdateStage updateStage READ updateStage NOTIFY updateStageChanged)
        Q_PROPERTY(int foundTracks READ foundTracks NOTIFY foundTracksChanged)
        Q_PROPERTY(int extractedTracks READ extractedTracks NOTIFY extractedTracksChanged)

        Q_PROPERTY(bool removingFiles READ isRemovingFiles NOTIFY removingFilesChanged)

        Q_PROPERTY(bool savingTags READ isSavingTags NOTIFY savingTagsChanged)
    public:
        using UpdateStage = LibraryUpdateRunnable::UpdateStage;

        static QSqlDatabase openDatabase(const QString& connectionName = QSqlDatabase::defaultConnection);

        static const QString databaseType;
        static const size_t maxDbVariableCount;
        static LibraryUtils* instance();

        static QString findMediaArtForDirectory(std::unordered_map<QString, QString>& mediaArtHash, const QString& directoryPath, const std::atomic_bool& cancelFlag = false);

        static bool removeTracksFromDbByIds(const std::vector<int>& ids, const QSqlDatabase& db, const std::atomic_bool& cancel = false);
        static void removeUnusedCategories(const QSqlDatabase& db);
        static void removeUnusedMediaArt(const QSqlDatabase& db, const QString& mediaArtDirectory, const std::atomic_bool& cancel = false);

        void initDatabase();
        Q_INVOKABLE void updateDatabase();
        Q_INVOKABLE void cancelDatabaseUpdate();
        Q_INVOKABLE void resetDatabase();

        bool isDatabaseInitialized() const;
        bool isCreatedTables() const;

        int artistsCount() const;
        int albumsCount() const;
        int tracksCount() const;
        int tracksDuration() const;
        QString randomMediaArt() const;
        Q_INVOKABLE QString randomMediaArtForArtist(const QString& artist) const;
        Q_INVOKABLE QString randomMediaArtForAlbum(const QString& artist, const QString& album) const;
        Q_INVOKABLE QString randomMediaArtForGenre(const QString& genre) const;

        Q_INVOKABLE void setMediaArt(const QString& artist, const QString& album, const QString& mediaArt);

        bool isUpdating() const;
        UpdateStage updateStage() const;
        int foundTracks() const;
        int extractedTracks() const;

        bool isRemovingFiles() const;
        void removeArtists(std::vector<int>&& artists, bool deleteFiles);
        void removeAlbums(std::vector<int>&& albums, bool deleteFiles);
        void removeGenres(std::vector<int>&& genres, bool deleteFiles);

        void removeTracks(std::vector<LibraryTrack>&& tracks, bool deleteFiles);
        void removeTracksByPaths(std::vector<QString>&& paths, bool deleteFiles, bool deleteDirectories);

        bool isSavingTags() const;
        Q_INVOKABLE void saveTags(const QStringList& files, const QVariantMap& tags, bool incrementTrackNumber);

        std::unordered_map<QByteArray, QString> getEmbeddedMediaArt();
        QString saveEmbeddedMediaArt(const QByteArray& data, std::unordered_map<QByteArray, QString>& embeddedMediaArtFiles, const QMimeDatabase& mimeDb);
    private:
        LibraryUtils(QObject* parent = nullptr);

        bool mDatabaseInitialized;
        bool mCreatedTables;

        QRunnable* mLibraryUpdateRunnable;
        UpdateStage mLibraryUpdateStage;
        int mFoundTracks;
        int mExtractedTracks;

        bool mRemovingFiles;
        bool mSavingTags;

        QString mMediaArtDirectory;
    signals:
        void updatingChanged();
        void updateStageChanged();
        void foundTracksChanged();
        void extractedTracksChanged();

        void databaseChanged();
        void mediaArtChanged();

        void removingFilesChanged();
        void savingTagsChanged();
    };
}

#endif // UNPLAYER_LIBRARYUTILS_H
