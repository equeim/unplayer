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

#include "settings.h"

#include <QCoreApplication>
#include <QSettings>
#include <QStandardPaths>

#include "utils.h"

namespace unplayer
{
    namespace
    {
        const QString libraryDirectoriesKey(QLatin1String("libraryDirectories"));
        const QString openLibraryOnStartupKey(QLatin1String("openLibraryOnStartup"));
        const QString defaultDirectoryKey(QLatin1String("defaultDirectory"));
        const QString useDirectoryMediaArtKey(QLatin1String("useDirectoryMediaArt"));
        const QString restorePlayerStateKey(QLatin1String("restorePlayerState"));

        const QString queueTracksKey(QLatin1String("state/queueTracks"));
        const QString queuePositionKey(QLatin1String("state/queuePosition"));
        const QString playerPositionKey(QLatin1String("state/playerPosition"));

        Settings* instancePointer = nullptr;
    }

    Settings* Settings::instance()
    {
        if (!instancePointer) {
            instancePointer = new Settings(qApp);
        }
        return instancePointer;
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

    QStringList Settings::queueTracks() const
    {
        return mSettings->value(queueTracksKey).toStringList();
    }

    int Settings::queuePosition() const
    {
        return mSettings->value(queuePositionKey).toInt();
    }

    long long Settings::playerPosition() const
    {
        return mSettings->value(playerPositionKey).toLongLong();
    }

    void Settings::savePlayerState(const QStringList& tracks, int queuePosition, long long playerPosition)
    {
        mSettings->setValue(queueTracksKey, tracks);
        mSettings->setValue(queuePositionKey, queuePosition);
        mSettings->setValue(playerPositionKey, playerPosition);
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
