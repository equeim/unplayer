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

#include <QObject>
#include <QStringList>

namespace unplayer
{
    struct PlaylistTrack
    {
        QString filePath;
        QString title;
        int duration;
        bool hasDuration;
        QString artist;
        QString album;
        bool inLibrary;
    };

    class PlaylistUtils final : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(int playlistsCount READ playlistsCount NOTIFY playlistsChanged)
    public:
        static const QStringList playlistsNameFilters;
        static const std::vector<QString> playlistsMimeTypes;

        static PlaylistUtils* instance();

        const QString& playlistsDirectoryPath();
        int playlistsCount();

        void savePlaylist(const QString& filePath, const std::vector<PlaylistTrack>& tracks);
        Q_INVOKABLE void newPlaylist(const QString& name, const QStringList& trackPaths);
        Q_INVOKABLE void addTracksToPlaylist(const QString& filePath, const QStringList& trackPaths);
        Q_INVOKABLE void removePlaylist(const QString& filePath);
        void removePlaylists(const QStringList& playlists);

        static std::vector<PlaylistTrack> parsePlaylist(const QString& filePath);
        static int getPlaylistTracksCount(const QString& filePath);
        Q_INVOKABLE static QStringList getPlaylistTracks(const QString& filePath);
    private:
        explicit PlaylistUtils(QObject* parent);

        QString mPlaylistsDirectoryPath;
    signals:
        void playlistsChanged();
    };
}

Q_DECLARE_TYPEINFO(unplayer::PlaylistTrack, Q_MOVABLE_TYPE);

#endif // UNPLAYER_PLAYLISTUTILS_H
