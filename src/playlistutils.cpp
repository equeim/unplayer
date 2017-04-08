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

        enum class PlaylistType
        {
            Pls,
            M3u,
            Other
        };

        PlaylistType playlistTypeFromPath(const QString& filePath)
        {
            if (filePath.endsWith(QLatin1String(".pls"))) {
                return PlaylistType::Pls;
            }
            if (filePath.endsWith(QLatin1String(".m3u"))) {
                return PlaylistType::M3u;
            }
            return PlaylistType::Other;
        }

        PlaylistTrack trackFromFilePath(const QString& filePath)
        {
            int duration = 0;
            bool hasDuration = false;
            QString artist;
            QString album;
            QString title;
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

            if (title.isEmpty()) {
                title = QFileInfo(filePath).fileName();
            }

            return PlaylistTrack{filePath,
                                 title,
                                 duration,
                                 hasDuration,
                                 artist,
                                 album,
                                 inLibrary};
        }

        QList<PlaylistTrack> tracksFromPaths(const QStringList& trackPaths)
        {
            QList<PlaylistTrack> tracks;
            for (const QString& filePath : trackPaths) {
                tracks.append(trackFromFilePath(filePath));
            }
            return tracks;
        }

        QString filePathFromString(const QString& string, const QDir& playlistFileDir)
        {
            const QUrl url(string);
            if (url.isLocalFile()) {
                // string is file:// URL
                return url.path();
            }
            if (!url.isRelative()) {
                // string is not file:// URL or file path
                return QString();
            }
            return playlistFileDir.absoluteFilePath(string);
        }
    }

    const QStringList PlaylistUtils::playlistsNameFilters{QLatin1String("*.pls"), QLatin1String("*.m3u")};
    const QVector<QString> PlaylistUtils::playlistsMimeTypes{QLatin1String("audio/x-scpls"),
                                                             QLatin1String("audio/x-mpegurl"),
                                                             QLatin1String("application/vnd.apple.mpegurl")};

    PlaylistUtils* PlaylistUtils::instance()
    {
        if (!instancePointer) {
            instancePointer = new PlaylistUtils(qApp);
        }
        return instancePointer;
    }

    const QString& PlaylistUtils::playlistsDirectoryPath()
    {
        return mPlaylistsDirectoryPath;
    }

    int PlaylistUtils::playlistsCount()
    {
        return QDir(mPlaylistsDirectoryPath).entryList(playlistsNameFilters, QDir::Files).size();
    }

    void PlaylistUtils::savePlaylist(const QString& filePath, const QList<PlaylistTrack>& tracks)
    {
        switch (playlistTypeFromPath(filePath)) {
        case PlaylistType::Pls:
        {
            QFile file(filePath);
            if (!file.open(QIODevice::WriteOnly)) {
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

            break;
        }
        case PlaylistType::M3u:
        {
            QFile file(filePath);
            if (!file.open(QIODevice::WriteOnly)) {
                qWarning() << "error opening playlist file:" << filePath << file.error() << file.errorString();
                return;
            }
            QTextStream stream(&file);

            stream << "#EXTM3U" << endl;
            for (const PlaylistTrack& track : tracks) {
                stream << endl;
                if (track.hasDuration) {
                    stream << "#EXTINF:" << track.duration << ", " << track.artist << " - " << track.title << endl;
                }
                stream << track.filePath << endl;
            }
            break;
        }
        case PlaylistType::Other:
            break;
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
        QDir playlistFileDir(QFileInfo(filePath).path());

        switch (playlistTypeFromPath(filePath)) {
        case PlaylistType::Pls:
        {
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
                    filePath = filePathFromString(file, playlistFileDir);
                    if (filePath.isEmpty()) {
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
            break;
        }
        case PlaylistType::M3u:
        {
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly)) {
                QTextStream stream(&file);
                while (!stream.atEnd()) {
                    const QString line(stream.readLine().trimmed());
                    if (line.startsWith(QLatin1String("#EXTINF"))) {
                        const int durationIndex = 8;
                        const int commaIndex = line.indexOf(',');
                        const int duration = line.midRef(durationIndex, commaIndex - durationIndex).toInt();
                        const int hyphenIndex = line.indexOf('-');
                        const QString artist(line.mid(commaIndex + 1, hyphenIndex - commaIndex - 1).trimmed());
                        const QString title(line.mid(hyphenIndex + 1).trimmed());

                        const QString filePath(filePathFromString(stream.readLine().trimmed(), playlistFileDir));
                        if (filePath.isEmpty()) {
                            continue;
                        }

                        PlaylistTrack track(trackFromFilePath(filePath));
                        if (!track.inLibrary) {
                            track.duration = duration;
                            track.hasDuration = true;
                            track.title = title;
                            track.artist = artist;
                        }
                        tracks.append(track);
                    } else if (!line.startsWith('#') && !line.isEmpty()) {
                        const QString filePath(filePathFromString(line, playlistFileDir));
                        if (!filePath.isEmpty()) {
                            tracks.append(trackFromFilePath(filePath));
                        }
                    }
                }
            } else {
                qDebug() << "failed to open playlist:" << filePath;
            }
            break;
        }
        case PlaylistType::Other:
            break;
        }

        return tracks;
    }

    int PlaylistUtils::getPlaylistTracksCount(const QString& filePath)
    {
        switch (playlistTypeFromPath(filePath)) {
        case PlaylistType::Pls:
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
        case PlaylistType::M3u:
        {
            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly)) {
                qDebug() << "failed to open playlist:" << filePath;
                return 0;
            }
            int count = 0;
            QTextStream stream(&file);
            while (!stream.atEnd()) {
                const QString line(stream.readLine());
                if (!line.trimmed().isEmpty() && !line.startsWith('#')) {
                    ++count;
                }
            }
            return count;
        }
        default:
            return 0;
        }
    }

    QStringList PlaylistUtils::getPlaylistTracks(const QString& filePath)
    {
        QStringList tracks;
        QDir playlistFileDir(QFileInfo(filePath).path());

        switch (playlistTypeFromPath(filePath)) {
        case PlaylistType::Pls:
        {
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
                const QString filePath(filePathFromString(file, playlistFileDir));
                if (!filePath.isEmpty()) {
                    tracks.append(filePath);
                }
            }
            break;
        }
        case PlaylistType::M3u:
        {
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly)) {
                QTextStream stream(&file);
                while (!stream.atEnd()) {
                    const QString line(stream.readLine().trimmed());
                    if (!line.isEmpty() && !line.startsWith('#')) {
                        const QString filePath(filePathFromString(line, playlistFileDir));
                        if (!filePath.isEmpty()) {
                            tracks.append(filePath);
                        }
                    }
                }
            }
            break;
        }
        case PlaylistType::Other:
            break;
        }

        return tracks;
    }

    PlaylistUtils::PlaylistUtils(QObject* parent)
        : QObject(parent),
          mPlaylistsDirectoryPath(QString::fromLatin1("%1/playlists").arg(QStandardPaths::writableLocation(QStandardPaths::MusicLocation)))
    {

    }
}
