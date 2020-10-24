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

#ifndef UNPLAYER_SETTINGS_H
#define UNPLAYER_SETTINGS_H

#include <QObject>
#include <QString>
#include <QStringList>

class QSettings;

namespace unplayer
{
    class Settings final : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(bool hasLibraryDirectories READ hasLibraryDirectories NOTIFY libraryDirectoriesChanged)
        Q_PROPERTY(bool openLibraryOnStartup READ openLibraryOnStartup WRITE setOpenLibraryOnStartup)
        Q_PROPERTY(QString defaultDirectory READ defaultDirectory WRITE setDefaultDirectory)
        Q_PROPERTY(bool useDirectoryMediaArt READ useDirectoryMediaArt WRITE setUseDirectoryMediaArt)
        Q_PROPERTY(bool restorePlayerState READ restorePlayerState WRITE setRestorePlayerState)
        Q_PROPERTY(bool showVideoFiles READ showVideoFiles WRITE setShowVideoFiles)
        Q_PROPERTY(bool showNowPlayingCodecInfo READ showNowPlayingCodecInfo WRITE setShowNowPlayingCodecInfo NOTIFY showNowPlayingCodecInfoChanged)
    public:
        static Settings* instance();

        bool hasLibraryDirectories() const;

        QStringList libraryDirectories() const;
        void setLibraryDirectories(const QStringList& directories);

        bool openLibraryOnStartup() const;
        void setOpenLibraryOnStartup(bool open);

        bool useAlbumArtist() const;
        void setUseAlbumArtist(bool use);

        QStringList blacklistedDirectories() const;
        void setBlacklistedDirectories(const QStringList& directories);

        QString defaultDirectory() const;
        void setDefaultDirectory(const QString& directory);

        bool useDirectoryMediaArt() const;
        void setUseDirectoryMediaArt(bool use);

        bool restorePlayerState() const;
        void setRestorePlayerState(bool restore);

        bool showVideoFiles() const;
        void setShowVideoFiles(bool show);

        bool showNowPlayingCodecInfo() const;
        void setShowNowPlayingCodecInfo(bool show);

        bool artistsSortDescending() const;
        void setArtistsSortDescending(bool descending);

        bool albumsSortDescending() const;
        int albumsSortMode(int defaultMode) const;
        void setAlbumsSortSettings(bool descending, int sortMode);

        bool allAlbumsSortDescending() const;
        int allAlbumsSortMode(int defaultMode) const;
        void setAllAlbumsSortSettings(bool descending, int sortMode);

        bool albumTracksSortDescending() const;
        int albumTracksSortMode(int defaultMode) const;
        void setAlbumTracksSortSettings(bool descending, int sortMode);

        bool artistTracksSortDescending() const;
        int artistTracksSortMode(int defaultMode) const;
        int artistTracksInsideAlbumSortMode(int defaultMode) const;
        void setArtistTracksSortSettings(bool descending, int sortMode, int insideAlbumSortMode);

        bool allTracksSortDescending() const;
        int allTracksSortMode(int defaultMode) const;
        int allTracksInsideAlbumSortMode(int defaultMode) const;
        void setAllTracksSortSettings(bool descending, int sortMode, int insideAlbumSortMode);

        bool genresSortDescending() const;
        void setGenresSortDescending(bool descending);

        QStringList queueTracks() const;
        int queuePosition() const;
        bool shuffle() const;
        int repeatMode() const;
        long long playerPosition() const;
        bool stopAfterEos() const;
        void savePlayerState(const QStringList& tracks,
                             int queuePosition,
                             bool shuffle,
                             int repeatMode,
                             long long playerPosition,
                             bool stopAfterEos);
    private:
        explicit Settings(QObject* parent);

        QSettings* mSettings;
    signals:
        void showNowPlayingCodecInfoChanged(bool show);
        void libraryDirectoriesChanged();
    };
}

#endif // UNPLAYER_SETTINGS_H
