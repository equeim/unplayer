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

#include "utils.h"

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

void PlaylistUtils::newPlaylist(const QString &name, const QVariant &tracksVariant)
{
    m_newPlaylist = true;
    m_newPlaylistUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::MusicLocation) +
                                           "/playlists/" +
                                           name +
                                           ".pls").toEncoded();

    QStringList tracks(unboxTracks(tracksVariant));

    m_newPlaylistTracksCount = tracks.size();
    savePlaylist(m_newPlaylistUrl, tracks);
}

void PlaylistUtils::addTracksToPlaylist(const QString &playlistUrl, const QVariant &newTracksVariant)
{
    QStringList tracks(parsePlaylist(playlistUrl));
    tracks.append(unboxTracks(newTracksVariant));
    savePlaylist(playlistUrl, tracks);
    setPlaylistTracksCount(playlistUrl, tracks.size());
}

void PlaylistUtils::removeTracksFromPlaylist(const QString &playlistUrl, const QList<int> &trackIndexes)
{
    QStringList tracks(parsePlaylist(playlistUrl));

    for (int i = 0, max = trackIndexes.size(); i < max; i++)
        tracks.removeAt(trackIndexes.at(i) - i);

    savePlaylist(playlistUrl, tracks);
    setPlaylistTracksCount(playlistUrl, tracks.size());
}

void PlaylistUtils::removePlaylist(const QString &url)
{
    QFile::remove(QUrl(url).path());
}

QVariantList PlaylistUtils::syncParsePlaylist(const QString &playlistUrl)
{
    QVariantList trackVariants;

    for (const QString &trackUrl : parsePlaylist(playlistUrl)) {
        QSparqlResult *result = m_sparqlConnection->syncExec(QSparqlQuery(trackSparqlQuery(trackUrl),
                                                                          QSparqlQuery::SelectStatement));

        if (result->next()) {
            QSparqlResultRow row(result->current());

            QVariantMap trackMap;
            trackMap.insert("title", row.value("title"));
            trackMap.insert("url", row.value("url"));
            trackMap.insert("duration", row.value("duration"));
            trackMap.insert("artist", row.value("artist"));
            trackMap.insert("rawArtist", row.value("rawArtist"));
            trackMap.insert("album", row.value("album"));
            trackMap.insert("rawAlbum", row.value("rawAlbum"));

            trackVariants.append(trackMap);
            result->deleteLater();
        }
    }

    return trackVariants;
}

QStringList PlaylistUtils::parsePlaylist(const QString &playlistUrl)
{
    QFile file(QUrl(playlistUrl).path());
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray line(file.readLine());
        if (line.trimmed() == "[playlist]") {
            QStringList tracks;
            while (!file.atEnd()) {
                line = file.readLine();
                if (line.startsWith("File")) {
                    int index = line.indexOf('=');
                    if (index >= 0) {
                        QByteArray url(line.mid(index + 1).trimmed());
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

QString PlaylistUtils::trackSparqlQuery(const QString &trackUrl)
{
    return QString("SELECT nie:url(?track) AS ?url\n"
                   "       tracker:coalesce(nie:title(?track), nfo:fileName(?track)) AS ?title\n"
                   "       tracker:coalesce(nmm:artistName(nmm:performer(?track)), \"" + tr("Unknown artist") + "\") AS ?artist\n"
                   "       nmm:artistName(nmm:performer(?track)) AS ?rawArtist\n"
                   "       tracker:coalesce(nie:title(nmm:musicAlbum(?track)), \"" + tr("Unknown album") + "\") AS ?album\n"
                   "       nie:title(nmm:musicAlbum(?track)) AS ?rawAlbum\n"
                   "       nfo:duration(?track) AS ?duration\n"
                   "WHERE {\n"
                   "    ?track nie:url \"" + Utils::encodeUrl(trackUrl) + "\".\n"
                   "}");
}

QStringList PlaylistUtils::unboxTracks(const QVariant &tracksVariant)
{
    if (tracksVariant.type() == QVariant::String)
        return QStringList({tracksVariant.toString()});

    if (tracksVariant.type() == QVariant::List) {
        QStringList tracks;
        for (const QVariant &trackVariant : tracksVariant.toList())
            tracks.append(trackVariant.toMap().value("url").toString());
        return tracks;
    }

    return QStringList();
}

void PlaylistUtils::setPlaylistTracksCount(QString playlistUrl, int tracksCount)
{
    playlistUrl = QUrl(playlistUrl).toEncoded();

    QSparqlResult *result = m_sparqlConnection->syncExec(QSparqlQuery(QString("DELETE {\n"
                                                                              "    ?playlist nfo:entryCounter ?tracksCount\n"
                                                                              "} WHERE {\n"
                                                                              "    ?playlist nie:url \"" + playlistUrl + "\";\n"
                                                                              "              nfo:entryCounter ?tracksCount.\n"
                                                                              "}\n"),
                                                                      QSparqlQuery::DeleteStatement));
    result->deleteLater();

    result = m_sparqlConnection->exec(QSparqlQuery(QString("INSERT {\n"
                                                           "    ?playlist nfo:entryCounter " + QString::number(tracksCount) + "\n"
                                                           "} WHERE {\n"
                                                           "    ?playlist nie:url \"" + playlistUrl + "\".\n"
                                                           "}\n"),
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

void PlaylistUtils::onTrackerGraphUpdated(const QString &className)
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
