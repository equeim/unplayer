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

class QRunnable;
class QSqlQuery;

namespace unplayer
{
    struct Album;

    namespace tagutils
    {
        struct Info;
    }

    enum class Extension
    {
        FLAC,
        AAC,

        M4A,
        MP3,

        OGG,
        OPUS,

        APE,
        MKA,

        WAV,
        WAVPACK,

        Other
    };

    QString mediaArtFromQuery(const QSqlQuery& query, int directoryMediaArtField, int embeddedMediaArtField);

    struct DatabaseGuard
    {
        ~DatabaseGuard();
        const QString connectionName;
    };

    struct CommitGuard
    {
        ~CommitGuard();
        QSqlDatabase& db;
    };

    class LibraryUtils final : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(bool databaseInitialized READ isDatabaseInitialized CONSTANT)
        Q_PROPERTY(bool createdTable READ isCreatedTable CONSTANT)

        Q_PROPERTY(int artistsCount READ artistsCount NOTIFY databaseChanged)
        Q_PROPERTY(int albumsCount READ albumsCount NOTIFY databaseChanged)
        Q_PROPERTY(int tracksCount READ tracksCount NOTIFY databaseChanged)
        Q_PROPERTY(int tracksDuration READ tracksDuration NOTIFY databaseChanged)
        Q_PROPERTY(QString randomMediaArt READ randomMediaArt NOTIFY mediaArtChanged)

        Q_PROPERTY(bool updating READ isUpdating NOTIFY updatingChanged)
        Q_PROPERTY(UpdateStage updateStage READ updateStage NOTIFY updateStageChanged)
        Q_PROPERTY(int foundTracks READ foundTracks NOTIFY foundTracksChanged)
        Q_PROPERTY(int extractedTracks READ extractedTracks NOTIFY extractedTracksChanged)

        Q_PROPERTY(bool removingFiles READ isRemovingFiles NOTIFY removingFilesChanged)

        Q_PROPERTY(QString titleTag READ titleTag CONSTANT)
        Q_PROPERTY(QString artistsTag READ artistsTag CONSTANT)
        Q_PROPERTY(QString albumArtistsTag READ albumArtistsTag CONSTANT)
        Q_PROPERTY(QString albumsTag READ albumsTag CONSTANT)
        Q_PROPERTY(QString yearTag READ yearTag CONSTANT)
        Q_PROPERTY(QString trackNumberTag READ trackNumberTag CONSTANT)
        Q_PROPERTY(QString genresTag READ genresTag CONSTANT)
        Q_PROPERTY(QString discNumberTag READ discNumberTag CONSTANT)
        Q_PROPERTY(bool savingTags READ isSavingTags NOTIFY savingTagsChanged)
    public:
        enum UpdateStage {
            NoneStage,
            PreparingStage,
            ScanningStage,
            ExtractingStage,
            FinishingStage
        };
        Q_ENUM(UpdateStage)

        static Extension extensionFromSuffix(const QString& suffix);
        static bool isExtensionSupported(const QString& suffix);
        static bool isVideoExtensionSupported(const QString& suffix);

        static QSqlDatabase openDatabase(const QString& connectionName = QSqlDatabase::defaultConnection);

        static const QString databaseType;
        static const int maxDbVariableCount;
        static LibraryUtils* instance();

        static QString findMediaArtForDirectory(std::unordered_map<QString, QString>& mediaArtHash, const QString& directoryPath, const std::atomic_bool& cancelFlag = false);

        void initDatabase();
        Q_INVOKABLE void updateDatabase();
        Q_INVOKABLE void cancelDatabaseUpdate();
        Q_INVOKABLE void resetDatabase();

        bool isDatabaseInitialized() const;
        bool isCreatedTable() const;

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
        void removeArtists(std::vector<QString>&& artists, bool deleteFiles);
        void removeAlbums(std::vector<Album>&& albums, bool deleteFiles);
        void removeGenres(std::vector<QString>&& genres, bool deleteFiles);
        void removeFiles(std::vector<QString>&& files, bool deleteFiles, bool canHaveDirectories);

        QString titleTag() const;
        QString artistsTag() const;
        QString albumArtistsTag() const;
        QString albumsTag() const;
        QString yearTag() const;
        QString trackNumberTag() const;
        QString genresTag() const;
        QString discNumberTag() const;
        bool isSavingTags() const;
        Q_INVOKABLE void saveTags(const QStringList& files, const QVariantMap& tags, bool incrementTrackNumber);
    private:
        LibraryUtils(QObject* parent = nullptr);

        bool mDatabaseInitialized;
        bool mCreatedTable;

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
