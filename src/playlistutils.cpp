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

#include "playlistutils.h"
#include "playlistutils.moc"

#include <QDBusConnection>
#include <QFile>
#include <QStandardPaths>
#include <QUrl>

#include <QSparqlConnection>
#include <QSparqlConnectionOptions>
#include <QSparqlQuery>
#include <QSparqlResult>

namespace Unplayer
{

PlaylistUtils::PlaylistUtils()
    : m_sparqlConnection(new QSparqlConnection("QTRACKER_DIRECT", QSparqlConnectionOptions(), this)),
      m_newPlaylist(false)
{
    QDBusConnection::sessionBus().connect("org.freedesktop.Tracker1",
                                          "/org/freedesktop/Tracker1/Resources",
                                          "org.freedesktop.Tracker1.Resources",
                                          "GraphUpdated",
                                          this,
                                          SLOT(onTrackerGraphUpdated(QString)));
}

void PlaylistUtils::newPlaylist(const QString &name, const QVariantList &tracks)
{
    m_newPlaylist = true;
    m_newPlaylistUrl = QUrl("file://" + QStandardPaths::writableLocation(QStandardPaths::MusicLocation) + "/playlists/" + name + ".pls").toEncoded();
    m_newPlaylistTracksCount = tracks.size();

    QStringList trackUrls;
    QVariantList::const_iterator iterator = tracks.cbegin();
    while (iterator != tracks.cend()) {
        trackUrls.append((*iterator).toMap().value("url").toString());
        iterator++;
    }

    savePlaylist(m_newPlaylistUrl, trackUrls);
}

void PlaylistUtils::addTracksToPlaylist(const QString &playlistUrl, const QVariantList &newTracks)
{
    QStringList tracks = parsePlaylist(playlistUrl);

    QVariantList::const_iterator iterator = newTracks.cbegin();
    while (iterator != newTracks.cend()) {
        tracks.append((*iterator).toMap().value("url").toString());
        iterator++;
    }

    savePlaylist(playlistUrl, tracks);
    setPlaylistTracksCount(playlistUrl, tracks.size());
}

void PlaylistUtils::removeTrackFromPlaylist(const QString &playlistUrl, int trackIndex)
{
    QStringList tracks = parsePlaylist(playlistUrl);
    tracks.removeAt(trackIndex);
    savePlaylist(playlistUrl, tracks);
    setPlaylistTracksCount(playlistUrl, tracks.size());
}

void PlaylistUtils::clearPlaylist(const QString &url)
{
    savePlaylist(url, QStringList());
    setPlaylistTracksCount(url, 0);
}

void PlaylistUtils::removePlaylist(const QString &url)
{
    QFile(QUrl(url).path()).remove();
}

QStringList PlaylistUtils::parsePlaylist(const QString &playlistUrl)
{
    QFile file(QUrl(playlistUrl).path());
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray line = file.readLine();
        if (line.trimmed() == "[playlist]") {
            QStringList tracks;
            while (!file.atEnd()) {
                line = file.readLine();
                if (line.startsWith("File")) {
                    int index = line.indexOf('=');
                    if (index >= 0) {
                        QByteArray url = line.mid(index + 1).trimmed();
                        if (!url.isEmpty())
                            tracks.append(url);
                    }
                }
            }
            return tracks;
        }
    }

    return QStringList();
}

void PlaylistUtils::setPlaylistTracksCount(const QString &playlistUrl, int tracksCount)
{
    QSparqlResult *result = m_sparqlConnection->syncExec(QSparqlQuery(QString("DELETE {\n"
                                             "    ?playlist nfo:entryCounter ?tracksCount\n"
                                             "} WHERE {\n"
                                             "    ?playlist nie:url \"%1\";\n"
                                             "              nfo:entryCounter ?tracksCount.\n"
                                             "}\n").arg(playlistUrl),
                                     QSparqlQuery::DeleteStatement));
    result->deleteLater();

    result = m_sparqlConnection->exec(QSparqlQuery(QString("INSERT {\n"
                                             "    ?playlist nfo:entryCounter %1\n"
                                             "} WHERE {\n"
                                             "    ?playlist nie:url \"%2\".\n"
                                             "}\n").arg(tracksCount).arg(playlistUrl),
                                     QSparqlQuery::InsertStatement));

    connect(result, &QSparqlResult::finished, result, &QObject::deleteLater);
}

void PlaylistUtils::savePlaylist(const QString &playlistUrl, const QStringList &tracks)
{
    QFile file(QUrl(playlistUrl).path());
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate))
        return;

    int tracksCount = tracks.size();

    file.write(QByteArray("[playlist]\nNumberOfEntries=") + QByteArray::number(tracksCount));

    for (int i = 0; i < tracksCount; i++) {
        file.write(QByteArray("\nFile") +
                   QByteArray::number(i + 1) +
                   "=" +
                   tracks.at(i).toUtf8());
    }

    file.close();
}

void PlaylistUtils::onTrackerGraphUpdated(QString className)
{
    if (className == "http://www.tracker-project.org/temp/nmm#Playlist") {
        emit playlistsChanged();

        if (m_newPlaylist) {
            setPlaylistTracksCount(m_newPlaylistUrl, m_newPlaylistTracksCount);
            m_newPlaylist = false;
        }
    }
}

}
