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

#include "settings.h"

#include <QCoreApplication>
#include <QSettings>
#include <QStandardPaths>

#include "utils.h"

namespace unplayer
{
    namespace
    {
        const QLatin1String libraryDirectoriesKey("libraryDirectories");
        const QLatin1String openLibraryOnStartupKey("openLibraryOnStartup");
        const QLatin1String blacklistedDirectoriesKey("blacklistedDirectories");
        const QLatin1String defaultDirectoryKey("defaultDirectory");
        const QLatin1String useDirectoryMediaArtKey("useDirectoryMediaArt");
        const QLatin1String restorePlayerStateKey("restorePlayerState");
        const QLatin1String showVideoFilesKey("showVideoFiles");
        const QLatin1String useAlbumArtistKey("useAlbumArtist");

        const QLatin1String artistsSortDescendingKey("artistsSortDescending");

        const QLatin1String albumsSortDescendingKey("albumsSortDescending");
        const QLatin1String albumsSortModeKey("albumsSortMode");

        const QLatin1String allAlbumsSortDescendingKey("allAlbumsSortDescending");
        const QLatin1String allAlbumsSortModeKey("allAlbumsSortMode");

        const QLatin1String albumTracksSortDescendingKey("albumTracksSortDescending");
        const QLatin1String albumTracksSortModeKey("albumTracksSortMode");

        const QLatin1String artistTracksSortDescendingKey("artistTracksSortDescending");
        const QLatin1String artistTracksSortModeKey("artistTracksSortMode");
        const QLatin1String artistTracksInsideAlbumSortModeKey("artistTracksInsideSortMode");

        const QLatin1String allTracksSortDescendingKey("allTracksSortDescending");
        const QLatin1String allTracksSortModeKey("allTracksSortMode");
        const QLatin1String allTracksInsideAlbumSortModeKey("allTracksInsideSortMode");

        const QLatin1String genresSortDescendingKey("genresSortDescending");

        const QLatin1String queueTracksKey("state/queueTracks");
        const QLatin1String queuePositionKey("state/queuePosition");
        const QLatin1String shuffleKey("state/shuffle");
        const QLatin1String repeatModeKey("state/repeatMode");
        const QLatin1String playerPositionKey("state/playerPosition");
        const QLatin1String stopAfterEosKey("state/stopAfterEos");
    }

    Settings* Settings::instance()
    {
        static const auto p = new Settings(qApp);
        return p;
    }

    bool Settings::hasLibraryDirectories() const
    {
        return !mSettings->value(libraryDirectoriesKey).toStringList().isEmpty();
    }

    QStringList Settings::libraryDirectories() const
    {
        return mSettings->value(libraryDirectoriesKey).toStringList();
    }

    void Settings::setLibraryDirectories(const QStringList& directories)
    {
        mSettings->setValue(libraryDirectoriesKey, directories);
        emit libraryDirectoriesChanged();
    }

    bool Settings::openLibraryOnStartup() const
    {
        return mSettings->value(openLibraryOnStartupKey, false).toBool();
    }

    void Settings::setOpenLibraryOnStartup(bool open)
    {
        mSettings->setValue(openLibraryOnStartupKey, open);
    }

    bool Settings::useAlbumArtist() const
    {
        return mSettings->value(useAlbumArtistKey, false).toBool();
    }

    void Settings::setUseAlbumArtist(bool use)
    {
        mSettings->setValue(useAlbumArtistKey, use);
    }

    QStringList Settings::blacklistedDirectories() const
    {
        return mSettings->value(blacklistedDirectoriesKey).toStringList();
    }

    void Settings::setBlacklistedDirectories(const QStringList& directories)
    {
        mSettings->setValue(blacklistedDirectoriesKey, directories);
    }

    QString Settings::defaultDirectory() const
    {
        return mSettings->value(defaultDirectoryKey, QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).toString();
    }

    void Settings::setDefaultDirectory(const QString& directory)
    {
        mSettings->setValue(defaultDirectoryKey, directory);
    }

    bool Settings::useDirectoryMediaArt() const
    {
        return mSettings->value(useDirectoryMediaArtKey, false).toBool();
    }

    void Settings::setUseDirectoryMediaArt(bool use)
    {
        mSettings->setValue(useDirectoryMediaArtKey, use);
    }

    bool Settings::restorePlayerState() const
    {
        return mSettings->value(restorePlayerStateKey, true).toBool();
    }

    void Settings::setRestorePlayerState(bool restore)
    {
        mSettings->setValue(restorePlayerStateKey, restore);
    }

    bool Settings::showVideoFiles() const
    {
        return mSettings->value(showVideoFilesKey, false).toBool();
    }

    void Settings::setShowVideoFiles(bool show)
    {
        mSettings->setValue(showVideoFilesKey, show);
    }

    bool Settings::artistsSortDescending() const
    {
        return mSettings->value(artistsSortDescendingKey, false).toBool();
    }

    void Settings::setArtistsSortDescending(bool descending)
    {
        mSettings->setValue(artistsSortDescendingKey, descending);
    }

    bool Settings::albumsSortDescending() const
    {
        return mSettings->value(albumsSortDescendingKey, false).toBool();
    }

    int Settings::albumsSortMode(int defaultMode) const
    {
        return mSettings->value(albumsSortModeKey, defaultMode).toInt();
    }

    void Settings::setAlbumsSortSettings(bool descending, int sortMode)
    {
        mSettings->setValue(albumsSortDescendingKey, descending);
        mSettings->setValue(albumsSortModeKey, sortMode);
    }

    bool Settings::allAlbumsSortDescending() const
    {
        return mSettings->value(allAlbumsSortDescendingKey, false).toBool();
    }

    int Settings::allAlbumsSortMode(int defaultMode) const
    {
        return mSettings->value(allAlbumsSortModeKey, defaultMode).toInt();
    }

    void Settings::setAllAlbumsSortSettings(bool descending, int sortMode)
    {
        mSettings->setValue(allAlbumsSortDescendingKey, descending);
        mSettings->setValue(allAlbumsSortModeKey, sortMode);
    }

    bool Settings::albumTracksSortDescending() const
    {
        return mSettings->value(albumTracksSortDescendingKey, false).toBool();
    }

    int Settings::albumTracksSortMode(int defaultMode) const
    {
        return mSettings->value(albumTracksSortModeKey, defaultMode).toInt();
    }

    void Settings::setAlbumTracksSortSettings(bool descending, int sortMode)
    {
        mSettings->setValue(albumTracksSortDescendingKey, descending);
        mSettings->setValue(albumTracksSortModeKey, sortMode);
    }

    bool Settings::artistTracksSortDescending() const
    {
        return mSettings->value(artistTracksSortDescendingKey, false).toBool();
    }

    int Settings::artistTracksSortMode(int defaultMode) const
    {
        return mSettings->value(artistTracksSortModeKey, defaultMode).toInt();
    }

    int Settings::artistTracksInsideAlbumSortMode(int defaultMode) const
    {
        return mSettings->value(artistTracksInsideAlbumSortModeKey, defaultMode).toInt();
    }

    void Settings::setArtistTracksSortSettings(bool descending, int sortMode, int insideAlbumSortMode)
    {
        mSettings->setValue(artistTracksSortDescendingKey, descending);
        mSettings->setValue(artistTracksSortModeKey, sortMode);
        mSettings->setValue(artistTracksInsideAlbumSortModeKey, insideAlbumSortMode);
    }

    bool Settings::allTracksSortDescending() const
    {
        return mSettings->value(allTracksSortDescendingKey, false).toBool();
    }

    int Settings::allTracksSortMode(int defaultMode) const
    {
        return mSettings->value(allTracksSortModeKey, defaultMode).toInt();
    }

    int Settings::allTracksInsideAlbumSortMode(int defaultMode) const
    {
        return mSettings->value(allTracksInsideAlbumSortModeKey, defaultMode).toInt();
    }

    void Settings::setAllTracksSortSettings(bool descending, int sortMode, int insideAlbumSortMode)
    {
        mSettings->setValue(allTracksSortDescendingKey, descending);
        mSettings->setValue(allTracksSortModeKey, sortMode);
        mSettings->setValue(allTracksInsideAlbumSortModeKey, insideAlbumSortMode);
    }

    bool Settings::genresSortDescending() const
    {
        return mSettings->value(genresSortDescendingKey, false).toBool();
    }

    void Settings::setGenresSortDescending(bool descending)
    {
        mSettings->setValue(genresSortDescendingKey, descending);
    }

    QStringList Settings::queueTracks() const
    {
        return mSettings->value(queueTracksKey).toStringList();
    }

    int Settings::queuePosition() const
    {
        return mSettings->value(queuePositionKey).toInt();
    }

    bool Settings::shuffle() const
    {
        return mSettings->value(shuffleKey).toBool();
    }

    int Settings::repeatMode() const
    {
        return mSettings->value(repeatModeKey).toInt();
    }

    long long Settings::playerPosition() const
    {
        return mSettings->value(playerPositionKey).toLongLong();
    }

    bool Settings::stopAfterEos() const
    {
        return mSettings->value(stopAfterEosKey).toBool();
    }

    void Settings::savePlayerState(const QStringList& tracks,
                                   int queuePosition,
                                   bool shuffle,
                                   int repeatMode,
                                   long long playerPosition,
                                   bool stopAfterEos)
    {
        mSettings->setValue(queueTracksKey, tracks);
        mSettings->setValue(queuePositionKey, queuePosition);
        mSettings->setValue(shuffleKey, shuffle);
        mSettings->setValue(repeatModeKey, repeatMode);
        mSettings->setValue(playerPositionKey, playerPosition);
        mSettings->setValue(stopAfterEosKey, stopAfterEos);
    }

    Settings::Settings(QObject* parent)
        : QObject(parent),
          mSettings(new QSettings(this))
    {
        if (!mSettings->contains(libraryDirectoriesKey)) {
            QStringList directories{QStandardPaths::writableLocation(QStandardPaths::MusicLocation)};
            const QString sdcardPath(Utils::sdcardPath(true));
            if (!sdcardPath.isEmpty()) {
                directories.append(sdcardPath);
            }
            setLibraryDirectories(directories);
        }
    }
}
