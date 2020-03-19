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

#include "playlistutils.h"

#include <algorithm>
#include <iterator>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QTextStream>
#include <QUrl>

#include "libraryutils.h"

namespace unplayer
{
    namespace
    {
        enum class PlaylistType
        {
            Pls,
            M3u,
            Other
        };

        const QLatin1String plsExtension("pls");

        const std::unordered_set<QString>& m3uExtensions()
        {
            static const std::unordered_set<QString> extensions{
                QLatin1String("m3u"),
                QLatin1String("m3u8"),
                QLatin1String("vlc")
            };
            return extensions;
        }

        inline bool isM3uExtension(const QString& suffix)
        {
            return contains(m3uExtensions(), suffix);
        }

        PlaylistType playlistTypeFromExtension(const QString& extension)
        {
            const QString lower(extension.toLower());
            if (lower == plsExtension) {
                return PlaylistType::Pls;
            }
            if (isM3uExtension(lower)) {
                return PlaylistType::M3u;
            }
            return PlaylistType::Other;
        }

        QUrl urlFromString(const QString& string, const QDir& playlistFileDir)
        {
            if (string.startsWith(QLatin1Char('/'))) {
                return QUrl::fromLocalFile(string);
            }
            const QUrl url(string);
            if (url.isRelative() || url.isLocalFile()) {
                return QUrl::fromLocalFile(playlistFileDir.absoluteFilePath(string));
            }
            return url;
        }

        void setTitleFromUrl(PlaylistTrack& track)
        {
            if (track.url.isLocalFile()) {
                track.title = QFileInfo(track.url.path()).fileName();
            } else {
                track.title = track.url.toString();
            }
        }

        namespace pls
        {
            void parse(std::vector<PlaylistTrack>& tracks, QTextStream& stream, const QString& filePath, QDir& playlistFileDir)
            {
                const QString firstNotEmpty([&stream]() {
                    while (!stream.atEnd()) {
                        const QString line(stream.readLine().trimmed());
                        if (!line.isEmpty() && !line.startsWith(QLatin1Char(';')) && !line.startsWith(QLatin1Char('#'))) {
                            return line;
                        }
                    }
                    return QString();
                }());

                if (firstNotEmpty != QLatin1String("[playlist]")) {
                    qWarning() << filePath << "is not a PLS playlist";
                    return;
                }

                enum class Key
                {
                    File,
                    Title,
                    Length,
                    Unknown
                };

                std::map<int, PlaylistTrack> tracksMap;

                while (!stream.atEnd()) {
                    const QString line(stream.readLine().trimmed());

                    const Key keyType = [&line] {
                        if (line.startsWith(QStringLiteral("File"))) {
                            return Key::File;
                        }
                        if (line.startsWith(QStringLiteral("Title"))) {
                            return Key::Title;
                        }
                        if (line.startsWith(QStringLiteral("Length"))) {
                            return Key::Length;
                        }
                        return Key::Unknown;
                    }();

                    if (keyType != Key::Unknown) {
                        const int numberIndex = [&keyType]() {
                            switch (keyType) {
                            case Key::File:
                                return 4;
                            case Key::Title:
                                return 5;
                            case Key::Length:
                                return 6;
                            default:
                                return 0;
                            }
                        }();
                        const int equalsIndex = line.indexOf(QLatin1Char('='));
                        if (equalsIndex != -1) {
                            bool ok;
                            const int number = line.midRef(numberIndex, equalsIndex - numberIndex).toInt(&ok);

                            if (ok) {
                                PlaylistTrack& track = [&number, &tracksMap]() {
                                    const auto found(tracksMap.find(number));
                                    if (found == tracksMap.end()) {
                                        PlaylistTrack t{};
                                        t.duration = -1;
                                        return tracksMap.insert({number, std::move(t)}).first;
                                    }
                                    return found;
                                }()->second;

                                const QStringRef value(line.midRef(equalsIndex + 1).trimmed());
                                switch (keyType) {
                                case Key::File:
                                    track.url = urlFromString(value.toString(), playlistFileDir);
                                    break;
                                case Key::Title:
                                    track.title = value.toString();
                                    break;
                                case Key::Length:
                                    track.duration = value.toInt();
                                    break;
                                case Key::Unknown:
                                    break;
                                }
                            }
                        }
                    }
                }
                tracks.reserve(tracksMap.size());
                for (auto& i : tracksMap) {
                    if (!i.second.url.isEmpty()) {
                        if (i.second.title.isEmpty()) {
                            setTitleFromUrl(i.second);
                        }
                        tracks.push_back(std::move(i.second));
                    }
                }
            }

            void getTrackUrls(QStringList& tracks, QTextStream& stream, QDir& playlistFileDir)
            {
                std::map<int, QString> tracksMap;
                while (!stream.atEnd()) {
                    const QString line(stream.readLine().trimmed());
                    if (line.startsWith(QStringLiteral("File"))) {
                        const int equalsIndex = line.indexOf(QLatin1Char('='));
                        if (equalsIndex != -1) {
                            bool ok;
                            const int number = line.midRef(4, equalsIndex - 4).toInt(&ok);
                            if (ok) {
                                const QUrl url(urlFromString(line.midRef(equalsIndex + 1).trimmed().toString(), playlistFileDir));
                                if (!url.isEmpty()) {
                                    tracksMap.insert({number, url.toString()});
                                }
                            }
                        }
                    }
                }
                tracks.reserve(static_cast<int>(tracksMap.size()));
                for (auto&& i : tracksMap) {
                    tracks.push_back(std::move(i).second);
                }
            }

            int getTracksCount(QTextStream& stream)
            {
                std::unordered_set<int> files;
                while (!stream.atEnd()) {
                    const QString line(stream.readLine().trimmed());
                    if (line.startsWith(QStringLiteral("File"))) {
                        const int equalsIndex = line.indexOf(QLatin1Char('='));
                        if (equalsIndex != -1) {
                            bool ok;
                            const int number = line.midRef(4, equalsIndex - 4).toInt(&ok);
                            if (ok) {
                                files.insert(number);
                            }
                        }
                    }
                }
                return static_cast<int>(files.size());
            }

            void save(QTextStream& stream, const std::vector<PlaylistTrack>& tracks)
            {
                stream << "[playlist]" << endl;

                for (size_t i = 0, max = tracks.size(); i < max; ++i) {
                    const PlaylistTrack& track = tracks[i];
                    const size_t number = i + 1;
                    stream << "File" << number << '=';
                    if (track.url.isLocalFile()) {
                        stream << track.url.path();
                    } else {
                        stream << track.url.toString();
                    }
                    stream << endl;
                    stream << "Title" << number << '=' << track.title << endl;
                    if (track.duration != -1) {
                        stream << "Length" << number << '=' << track.duration << endl;
                    }
                }

                stream << "NumberOfEntries=" << tracks.size() << endl;
            }
        }

        namespace m3u
        {
            void parse(std::vector<PlaylistTrack>& tracks, QTextStream& stream, QDir& playlistFileDir)
            {
                while (!stream.atEnd()) {
                    const QString line(stream.readLine().trimmed());
                    if (line.startsWith(QStringLiteral("#EXTINF:"))) {
                        QUrl url(urlFromString(stream.readLine().trimmed(), playlistFileDir));
                        if (url.isEmpty()) {
                            continue;
                        }

                        PlaylistTrack track;
                        track.url = std::move(url);
                        track.duration = -1;

                        const int durationIndex = 8;
                        const int commaIndex = line.indexOf(QLatin1Char(','), durationIndex);
                        if (commaIndex == -1) {
                            setTitleFromUrl(track);
                            continue;
                        }
                        const QStringRef durationRef(line.midRef(durationIndex, commaIndex - durationIndex));
                        if (!durationRef.isEmpty()) {
                            const int duration = durationRef.toInt();
                            if (duration != -1) {
                                track.duration = duration;
                            }
                        }

                        const int hyphenIndex = line.indexOf(QLatin1Char('-'), commaIndex + 1);
                        if (hyphenIndex == -1) {
                            track.title = line.midRef(commaIndex + 1).trimmed().toString();
                        } else {
                            track.artist = line.midRef(commaIndex + 1, hyphenIndex - commaIndex - 1).trimmed().toString();
                            track.title = line.midRef(hyphenIndex + 1).trimmed().toString();
                        }

                        tracks.push_back(std::move(track));
                    } else if (!line.startsWith(QLatin1Char('#')) && !line.isEmpty()) {
                        QUrl url(urlFromString(line, playlistFileDir));
                        if (!url.isEmpty()) {
                            PlaylistTrack track;
                            track.url = std::move(url);
                            track.duration = -1;
                            tracks.push_back(std::move(track));
                        }
                    }
                }
            }

            void getTrackUrls(QStringList& tracks, QTextStream& stream, QDir& playlistFileDir)
            {
                while (!stream.atEnd()) {
                    const QString line(stream.readLine().trimmed());
                    if (!line.isEmpty() && !line.startsWith(QLatin1Char('#'))) {
                        const QUrl url(urlFromString(line, playlistFileDir));
                        if (!url.isEmpty()) {
                            tracks.push_back(url.toString());
                        }
                    }
                }
            }

            int getTracksCount(QTextStream& stream)
            {
                int count = 0;
                while (!stream.atEnd()) {
                    const QString line(stream.readLine().trimmed());
                    if (!line.isEmpty() && !line.startsWith(QLatin1Char('#'))) {
                        ++count;
                    }
                }
                return count;
            }

            void save(QTextStream& stream, const std::vector<PlaylistTrack>& tracks)
            {
                stream << "#EXTM3U" << endl;
                for (const PlaylistTrack& track : tracks) {
                    if (!track.title.isEmpty()) {
                        stream << endl;
                        stream << "#EXTINF:" << track.duration << ", ";
                        if (track.artist.isEmpty()) {
                            stream << track.title;
                        } else {
                            stream << track.artist << " - " << track.title << endl;
                        }
                    }
                    if (track.url.isLocalFile()) {
                        stream << track.url.path();
                    } else {
                        stream << track.url.toString();
                    }
                    stream << endl;
                }
            }
        }

        PlaylistTrack trackFromUrlString(const QString& urlString)
        {
            int duration = -1;
            QString artist;
            QString album;
            QString title;

            const QUrl url(urlString);
            if (url.isRelative() || url.isLocalFile()) {
                QSqlQuery query;
                query.prepare(QLatin1String("SELECT title, duration, artist, album FROM tracks WHERE filePath = ?"));
                query.addBindValue(url.path());
                if (query.exec()) {
                    if (query.next()) {
                        title = query.value(0).toString();
                        duration = query.value(1).toInt();
                        artist = query.value(2).toString();
                        album = query.value(3).toString();
                    }
                } else {
                    qWarning() << "failed to get track from database:" << query.lastError();
                }
            }

            return PlaylistTrack{url,
                                 title,
                                 duration,
                                 artist,
                                 album};
        }

        std::vector<PlaylistTrack> tracksFromUrls(const QStringList& trackUrls)
        {
            std::vector<PlaylistTrack> tracks;
            tracks.reserve(trackUrls.size());
            for (const QString& urlString : trackUrls) {
                tracks.push_back(trackFromUrlString(urlString));
            }
            return tracks;
        }

        std::vector<PlaylistTrack> tracksFromTracks(const std::vector<LibraryTrack>& libraryTracks)
        {
            std::vector<PlaylistTrack> tracks;
            tracks.reserve(libraryTracks.size());
            for (const LibraryTrack& track : libraryTracks) {
                tracks.push_back({track.filePath,
                                  track.title,
                                  track.duration,
                                  track.artist,
                                  track.album});
            }
            return tracks;
        }
    }

    const QStringList& PlaylistUtils::playlistsNameFilters()
    {
        static const QStringList filters([]() {
            QStringList f;
            f.reserve(static_cast<int>(m3uExtensions().size() + 1));
            for (const QString& extension : m3uExtensions()) {
                f.push_back(QLatin1String("*.") % extension);
            }
            f.push_back(QLatin1String("*.") % plsExtension);
            return f;
        }());
        return filters;
    }

    bool PlaylistUtils::isPlaylistExtension(const QString& suffix)
    {
        static const std::unordered_set<QString> extensions([]() {
            static std::unordered_set<QString> ex(m3uExtensions());
            ex.insert(plsExtension);
            return ex;
        }());
        return contains(extensions, suffix.toLower());
    }

    PlaylistUtils* PlaylistUtils::instance()
    {
        static const auto p = new PlaylistUtils(qApp);
        return p;
    }

    const QString& PlaylistUtils::playlistsDirectoryPath()
    {
        return mPlaylistsDirectoryPath;
    }

    int PlaylistUtils::playlistsCount()
    {
        return QDir(mPlaylistsDirectoryPath).entryList(playlistsNameFilters(), QDir::Files).size();
    }

    void PlaylistUtils::savePlaylist(const QString& filePath, const std::vector<PlaylistTrack> &tracks)
    {
        if (!QDir().mkpath(mPlaylistsDirectoryPath)) {
            qWarning() << "failed to create playlists directory";
            return;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << "error opening playlist file:" << filePath << file.error() << file.errorString();
            return;
        }

        QTextStream stream(&file);

        switch (playlistTypeFromExtension(QFileInfo(filePath).suffix())) {
        case PlaylistType::Pls:
        {
            pls::save(stream, tracks);
            break;
        }
        case PlaylistType::M3u:
        {
            m3u::save(stream, tracks);
            break;
        }
        case PlaylistType::Other:
            break;
        }

        emit playlistsChanged();
    }

    void PlaylistUtils::newPlaylistFromFilesystem(const QString& name, const QStringList& trackUrls)
    {
        newPlaylist(name, tracksFromUrls(trackUrls));
    }

    void PlaylistUtils::newPlaylistFromLibrary(const QString& name, const std::vector<LibraryTrack>& libraryTracks)
    {
        newPlaylist(name, tracksFromTracks(libraryTracks));
    }

    void PlaylistUtils::newPlaylistFromLibrary(const QString& name, const LibraryTrack& libraryTracks)
    {
        newPlaylist(name, tracksFromTracks({libraryTracks}));
    }

    void PlaylistUtils::addTracksToPlaylistFromFilesystem(const QString& filePath, const QStringList& trackUrls)
    {
        addTracksToPlaylist(filePath, tracksFromUrls(trackUrls));
    }

    void PlaylistUtils::addTracksToPlaylistFromLibrary(const QString& filePath, const std::vector<LibraryTrack>& libraryTracks)
    {
        addTracksToPlaylist(filePath, tracksFromTracks(libraryTracks));
    }

    void PlaylistUtils::addTracksToPlaylistFromLibrary(const QString& filePath, const LibraryTrack& libraryTrack)
    {
        addTracksToPlaylist(filePath, tracksFromTracks({libraryTrack}));
    }

    void PlaylistUtils::removePlaylist(const QString& filePath)
    {
        if (QFile::remove(filePath)) {
            emit playlistsChanged();
        } else {
            qWarning() << "failed to remove playlist:" << filePath;
        }
    }

    void PlaylistUtils::removePlaylists(const std::vector<QString>& playlists)
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

    std::vector<PlaylistTrack> PlaylistUtils::parsePlaylist(const QString& filePath)
    {
        std::vector<PlaylistTrack> tracks;
        const QFileInfo fileInfo(filePath);
        QDir playlistFileDir(fileInfo.path());

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "error opening playlist file:" << filePath << file.error() << file.errorString();
            return tracks;
        }

        QTextStream stream(&file);

        switch (playlistTypeFromExtension(fileInfo.suffix())) {
        case PlaylistType::Pls:
        {
            pls::parse(tracks, stream, filePath, playlistFileDir);
            break;
        }
        case PlaylistType::M3u:
        {
            m3u::parse(tracks, stream, playlistFileDir);
            break;
        }
        case PlaylistType::Other:
            break;
        }

        return tracks;
    }

    QStringList PlaylistUtils::getPlaylistTracks(const QString& filePath)
    {
        QStringList tracks;
        QDir playlistFileDir(QFileInfo(filePath).path());

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "error opening playlist file:" << filePath << file.error() << file.errorString();
            return tracks;
        }

        QTextStream stream(&file);

        switch (playlistTypeFromExtension(QFileInfo(filePath).suffix())) {
        case PlaylistType::Pls:
        {
            pls::getTrackUrls(tracks, stream, playlistFileDir);
            break;
        }
        case PlaylistType::M3u:
        {
            m3u::getTrackUrls(tracks, stream, playlistFileDir);
            break;
        }
        case PlaylistType::Other:
            break;
        }

        return tracks;
    }

    int PlaylistUtils::getPlaylistTracksCount(const QString& filePath)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "error opening playlist file:" << filePath << file.error() << file.errorString();
            return 0;
        }

        QTextStream stream(&file);

        switch (playlistTypeFromExtension(QFileInfo(filePath).suffix())) {
        case PlaylistType::Pls:
        {
            return pls::getTracksCount(stream);
        }
        case PlaylistType::M3u:
        {
            return m3u::getTracksCount(stream);
        }
        default:
            return 0;
        }
    }

    PlaylistUtils::PlaylistUtils(QObject* parent)
        : QObject(parent),
          mPlaylistsDirectoryPath(QString::fromLatin1("%1/playlists").arg(QStandardPaths::writableLocation(QStandardPaths::MusicLocation)))
    {

    }

    void PlaylistUtils::newPlaylist(const QString& name, const std::vector<PlaylistTrack>& tracks)
    {
        savePlaylist(QString::fromLatin1("%1/%2.%3").arg(mPlaylistsDirectoryPath, name, plsExtension), tracks);
    }

    void PlaylistUtils::addTracksToPlaylist(const QString& filePath, std::vector<PlaylistTrack>&& tracks)
    {
        std::vector<PlaylistTrack> playlistTracks(parsePlaylist(filePath));
        playlistTracks.insert(playlistTracks.begin(), std::make_move_iterator(tracks.begin()), std::make_move_iterator(tracks.end()));
        savePlaylist(filePath, playlistTracks);
    }
}
