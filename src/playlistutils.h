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
#include <QVariant>

class QSparqlConnection;

namespace unplayer
{

class PlaylistUtils : public QObject
{
    Q_OBJECT
public:
    PlaylistUtils();

    Q_INVOKABLE void newPlaylist(const QString &name, const QVariant &tracksVariant);
    Q_INVOKABLE void addTracksToPlaylist(const QString &playlistUrl, const QVariant &newTracksVariant);
    Q_INVOKABLE void removeTracksFromPlaylist(const QString &playlistUrl, const QList<int> &trackIndexes);
    Q_INVOKABLE static void removePlaylist(const QString &url);
    Q_INVOKABLE QVariantList syncParsePlaylist(const QString &playlistUrl);
    static QStringList parsePlaylist(const QString &playlistUrl);
    static QString trackSparqlQuery(const QString &trackUrl);
private:
    QStringList unboxTracks(const QVariant &tracksVariant);
    void setPlaylistTracksCount(QString playlistUrl, int tracksCount);
    static void savePlaylist(const QString &playlistUrl, const QStringList &tracks);
private slots:
    void onTrackerGraphUpdated(const QString &className);
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
