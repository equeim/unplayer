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

#include "playlistutils.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>

#include <QFileInfo>
#include <QUrl>
#include <QSqlQuery>
#include <QSqlError>
#include <QSettings>

#include <QFile>

namespace unplayer
{
    namespace
    {
        PlaylistUtils* instancePointer = nullptr;
    }

    PlaylistUtils* PlaylistUtils::instance()
    {
        if (!instancePointer) {
            instancePointer = new PlaylistUtils(qApp);
        }
        return instancePointer;
    }

    QString PlaylistUtils::playlistsDirectoryPath()
    {
        return QString::fromLatin1("%1/playlists").arg(QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
    }

    int PlaylistUtils::playlistsCount()
    {
        return QDir(playlistsDirectoryPath()).entryList({QLatin1String("*.pls")}, QDir::Files).size();
    }

    void PlaylistUtils::savePlaylist(const QString& filePath, const QList<PlaylistTrack>& tracks)
    {
        {
            QFile file(filePath);
            if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
                qWarning() << "error opening playlist file:" << filePath << file.error() << file.errorString();
                return;
            }

            QTextStream stream(&file);

            stream << "[playlist]" << endl;

            for (int i = 0, max = tracks.size(); i < max; ++i) {
                const PlaylistTrack& track = tracks.at(i);
                stream << "File" << i + 1 << '=' << track.filePath << endl;
                stream << "Title" << i + 1 << '=' << track.title << endl;
                if (track.hasDuration) {
                    stream << "Length" << i + 1 << '=' << track.duration << endl;
                }
            }

            stream << "NumberOfEntries=" << tracks.size() << endl;
        }
        emit playlistsChanged();
    }

    void PlaylistUtils::newPlaylist(const QString& name, const QStringList& trackPaths)
    {
        savePlaylist(QString::fromLatin1("%1/%2.pls").arg(playlistsDirectoryPath()).arg(name), tracksFromPaths(trackPaths));
    }

    void PlaylistUtils::addTracksToPlaylist(const QString& filePath, const QStringList& trackPaths)
    {
        QList<PlaylistTrack> tracks(parsePlaylist(filePath));
        tracks.append(tracksFromPaths(trackPaths));
        savePlaylist(filePath, tracks);
    }

    void PlaylistUtils::removePlaylist(const QString& filePath)
    {
        if (QFile::remove(filePath)) {
            emit playlistsChanged();
        } else {
            qWarning() << "failed to remove playlist:" << filePath;
        }
    }

    void PlaylistUtils::removePlaylists(const QStringList& playlists)
    {
        bool removed = false;
        for (const QString& filePath : playlists) {
            if (QFile::remove(filePath)) {
                removed = true;
            } else {
                qWarning() << "failed to remove playlist:" << filePath;
            }
        }
        if (removed) {
            emit playlistsChanged();
        }
    }

    QList<PlaylistTrack> PlaylistUtils::parsePlaylist(const QString& filePath)
    {
        QList<PlaylistTrack> tracks;

        QSettings settings(filePath, QSettings::IniFormat);
        settings.beginGroup(QLatin1String("playlist"));

        QSqlDatabase::database().transaction();
        for (int i = 1, max = settings.childKeys().filter(QLatin1String("File")).size() + 1; i < max; ++i) {
            QString filePath;
            {
                QString file;
                const QVariant fileVariant(settings.value(QStringLiteral("File%1").arg(i)));
                if (fileVariant.type() == QVariant::StringList) {
                    file = fileVariant.toStringList().join(',');
                } else {
                    file = fileVariant.toString();
                }
                const QUrl url(QUrl::fromUserInput(file));
                if (url.isLocalFile()) {
                    filePath = url.path();
                } else {
                    continue;
                }
            }

            PlaylistTrack track(trackFromFilePath(filePath));

            if (!track.inLibrary) {
                const QString titleKey(QStringLiteral("Title%1").arg(i));
                if (settings.contains(titleKey)) {
                    const QVariant titleVariant(settings.value(titleKey));
                    if (titleVariant.type() == QVariant::StringList) {
                        track.title = titleVariant.toStringList().join(',');
                    } else {
                        track.title = titleVariant.toString();
                    }
                }
                if (track.title.isEmpty()) {
                    track.title = QFileInfo(filePath).fileName();
                }

                const QString lengthKey(QStringLiteral("Length%1").arg(i));
                if (settings.contains(lengthKey)) {
                    track.duration = settings.value(lengthKey).toInt();
                    track.hasDuration = true;
                }
            }

            tracks.append(track);
        }
        QSqlDatabase::database().commit();

        settings.endGroup();

        return tracks;
    }

    int PlaylistUtils::getPlaylistTracksCount(const QString& filePath)
    {
        QSettings settings(filePath, QSettings::IniFormat);
        settings.beginGroup(QStringLiteral("playlist"));
        int tracksCount;
        if (settings.contains(QStringLiteral("NumberOfEntries"))) {
            tracksCount = settings.value(QStringLiteral("NumberOfEntries")).toInt();
        } else {
            tracksCount = settings.childKeys().filter(QStringLiteral("File")).size();
        }
        settings.endGroup();
        return tracksCount;
    }

    QStringList PlaylistUtils::getPlaylistTracks(const QString& filePath)
    {
        QStringList tracks;
        QSettings settings(filePath, QSettings::IniFormat);
        settings.beginGroup(QStringLiteral("playlist"));
        for (int i = 1, max = settings.childKeys().filter(QStringLiteral("File")).size() + 1; i < max; ++i) {
            QString file;
            const QVariant fileVariant(settings.value(QStringLiteral("File%1").arg(i)));
            if (fileVariant.type() == QVariant::StringList) {
                file = fileVariant.toStringList().join(',');
            } else {
                file = fileVariant.toString();
            }
            const QUrl url(QUrl::fromUserInput(file));
            if (url.isLocalFile()) {
                tracks.append(url.path());
            } else {
                continue;
            }
        }
        return tracks;
    }

    PlaylistUtils::PlaylistUtils(QObject* parent)
        : QObject(parent)
    {

    }

    QList<PlaylistTrack> PlaylistUtils::tracksFromPaths(const QStringList& trackPaths)
    {
        QList<PlaylistTrack> tracks;
        for (const QString& filePath : trackPaths) {
            tracks.append(trackFromFilePath(filePath));
        }
        return tracks;
    }

    PlaylistTrack PlaylistUtils::trackFromFilePath(const QString& filePath)
    {
        QString title;
        int duration = 0;
        bool hasDuration = false;
        QString artist;
        QString album;
        bool inLibrary = false;

        QSqlQuery query;
        query.prepare(QLatin1String("SELECT title, duration, artist, album FROM tracks WHERE filePath = ?"));
        query.addBindValue(filePath);
        if (query.exec()) {
            if (query.next()) {
                title = query.value(0).toString();
                duration = query.value(1).toInt();
                hasDuration = true;
                artist = query.value(2).toString();
                album = query.value(3).toString();
                inLibrary = true;
            }
        } else {
            qWarning() << "failed to get track from database:" << query.lastError();
        }

        return PlaylistTrack{filePath,
                             title,
                             duration,
                             hasDuration,
                             artist,
                             album,
                             inLibrary};
    }
}
