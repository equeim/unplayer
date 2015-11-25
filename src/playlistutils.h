/*
 * Unplayer
 * Copyright (C) 2015 Alexey Rochev <equeim@gmail.com>
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

#include <QObject>

class QSparqlConnection;

namespace Unplayer
{

class PlaylistUtils : public QObject
{
    Q_OBJECT
public:
    PlaylistUtils();

    Q_INVOKABLE void newPlaylist(const QString &name, const QVariantList &tracks);
    Q_INVOKABLE void addTracksToPlaylist(const QString &playlistUrl, const QVariantList &newTracks);
    Q_INVOKABLE void removeTrackFromPlaylist(const QString &playlistUrl, int trackIndex);
    Q_INVOKABLE void clearPlaylist(const QString &url);
    Q_INVOKABLE static void removePlaylist(const QString &url);
    static QStringList parsePlaylist(const QString &playlistUrl);
private:
    void setPlaylistTracksCount(const QString &playlistUrl, int tracksCount);
    static void savePlaylist(const QString &playlistUrl, const QStringList &tracks);
private slots:
    void onTrackerGraphUpdated(QString className);
private:
    QSparqlConnection *m_sparqlConnection;

    bool m_newPlaylist;
    QString m_newPlaylistUrl;
    int m_newPlaylistTracksCount;
signals:
    void playlistsChanged();
};

}

#endif // UNPLAYER_PLAYLISTUTILS_H