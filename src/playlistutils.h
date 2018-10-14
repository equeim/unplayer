/*
 * Unplayer
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
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

#ifndef UNPLAYER_PLAYLISTUTILS_H
#define UNPLAYER_PLAYLISTUTILS_H

#include <memory>
#include <vector>
#include <unordered_set>

#include <QObject>
#include <QStringList>
#include <QUrl>

#include "librarytrack.h"
#include "stdutils.h"

namespace unplayer
{
    struct PlaylistTrack
    {
        QUrl url;
        QString title;
        int duration;
        QString artist;
        QString album;
    };

    class PlaylistUtils final : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(int playlistsCount READ playlistsCount NOTIFY playlistsChanged)
    public:
        static const QStringList playlistsNameFilters;
        static const std::unordered_set<QString> playlistsExtensions;

        static PlaylistUtils* instance();

        const QString& playlistsDirectoryPath();
        int playlistsCount();

        void savePlaylist(const QString& filePath, const std::vector<PlaylistTrack>& tracks);

        Q_INVOKABLE void newPlaylistFromFilesystem(const QString& name, const QStringList& trackUrls);
        Q_INVOKABLE void newPlaylistFromLibrary(const QString& name, const std::vector<unplayer::LibraryTrack>& libraryTracks);
        Q_INVOKABLE void newPlaylistFromLibrary(const QString& name, const unplayer::LibraryTrack& libraryTrack);

        Q_INVOKABLE void addTracksToPlaylistFromFilesystem(const QString& filePath, const QStringList& trackUrls);
        Q_INVOKABLE void addTracksToPlaylistFromLibrary(const QString& filePath, const std::vector<unplayer::LibraryTrack>& libraryTracks);
        Q_INVOKABLE void addTracksToPlaylistFromLibrary(const QString& filePath, const unplayer::LibraryTrack& libraryTrack);

        Q_INVOKABLE void removePlaylist(const QString& filePath);

        void removePlaylists(const std::vector<QString>& playlists);

        static std::vector<PlaylistTrack> parsePlaylist(const QString& filePath);
        Q_INVOKABLE static QStringList getPlaylistTracks(const QString& filePath);
        static int getPlaylistTracksCount(const QString& filePath);
    private:
        explicit PlaylistUtils(QObject* parent);

        void newPlaylist(const QString& name, const std::vector<PlaylistTrack>& tracks);
        void addTracksToPlaylist(const QString& filePath, std::vector<PlaylistTrack>&& tracks);

        QString mPlaylistsDirectoryPath;
    signals:
        void playlistsChanged();
    };
}

Q_DECLARE_TYPEINFO(unplayer::PlaylistTrack, Q_MOVABLE_TYPE);

#endif // UNPLAYER_PLAYLISTUTILS_H
