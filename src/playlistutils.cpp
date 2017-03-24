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

#include <QDBusConnection>
#include <QFile>
#include <QStandardPaths>
#include <QUrl>

#include <QSparqlConnection>
#include <QSparqlConnectionOptions>
#include <QSparqlQuery>
#include <QSparqlResult>

#include "utils.h"

namespace unplayer
{
    PlaylistUtils::PlaylistUtils()
        : mSparqlConnection(new QSparqlConnection(QStringLiteral("QTRACKER_DIRECT"), QSparqlConnectionOptions(), this)),
          mNewPlaylist(false)
    {
        QDBusConnection::sessionBus().connect(QStringLiteral("org.freedesktop.Tracker1"),
                                              QStringLiteral("/org/freedesktop/Tracker1/Resources"),
                                              QStringLiteral("org.freedesktop.Tracker1.Resources"),
                                              QStringLiteral("GraphUpdated"),
                                              this,
                                              SLOT(onTrackerGraphUpdated(QString)));
    }

    void PlaylistUtils::newPlaylist(const QString& name, const QVariant& tracksVariant)
    {
        mNewPlaylist = true;
        mNewPlaylistUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::MusicLocation) +
                                              "/playlists/" +
                                              name +
                                              ".pls")
                              .toEncoded();

        QStringList tracks(unboxTracks(tracksVariant));

        mNewPlaylistTracksCount = tracks.size();
        savePlaylist(mNewPlaylistUrl, tracks);
    }

    void PlaylistUtils::addTracksToPlaylist(const QString& playlistUrl, const QVariant& newTracksVariant)
    {
        QStringList tracks(parsePlaylist(playlistUrl));
        tracks.append(unboxTracks(newTracksVariant));
        savePlaylist(playlistUrl, tracks);
        setPlaylistTracksCount(playlistUrl, tracks.size());
    }

    void PlaylistUtils::removeTracksFromPlaylist(const QString& playlistUrl, const QList<int>& trackIndexes)
    {
        QStringList tracks(parsePlaylist(playlistUrl));

        for (int i = 0, max = trackIndexes.size(); i < max; i++)
            tracks.removeAt(trackIndexes.at(i) - i);

        savePlaylist(playlistUrl, tracks);
        setPlaylistTracksCount(playlistUrl, tracks.size());
    }

    void PlaylistUtils::removePlaylist(const QString& url)
    {
        QFile::remove(QUrl(url).path());
    }

    QVariantList PlaylistUtils::syncParsePlaylist(const QString& playlistUrl)
    {
        QVariantList trackVariants;

        for (const QString& trackUrl : parsePlaylist(playlistUrl)) {
            QSparqlResult* result = mSparqlConnection->syncExec(QSparqlQuery(trackSparqlQuery(trackUrl),
                                                                             QSparqlQuery::SelectStatement));

            if (result->next()) {
                QSparqlResultRow row(result->current());

                trackVariants.append(QVariantMap{{QStringLiteral("title"), row.value(QStringLiteral("title"))},
                                                 {QStringLiteral("url"), row.value(QStringLiteral("url"))},
                                                 {QStringLiteral("duration"), row.value(QStringLiteral("duration"))},
                                                 {QStringLiteral("artist"), row.value(QStringLiteral("artist"))},
                                                 {QStringLiteral("rawArtist"), row.value(QStringLiteral("rawArtist"))},
                                                 {QStringLiteral("album"), row.value(QStringLiteral("album"))},
                                                 {QStringLiteral("rawAlbum"), row.value(QStringLiteral("rawAlbum"))}});
                result->deleteLater();
            }
        }

        return trackVariants;
    }

    QStringList PlaylistUtils::parsePlaylist(const QString& playlistUrl)
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
                            if (!url.isEmpty()) {
                                tracks.append(url);
                            }
                        }
                    }
                }
                return tracks;
            }
        }

        return QStringList();
    }

    QString PlaylistUtils::trackSparqlQuery(const QString& trackUrl)
    {
        return QStringLiteral("SELECT nie:url(?track) AS ?url\n"
                              "       tracker:coalesce(nie:title(?track), nfo:fileName(?track)) AS ?title\n"
                              "       tracker:coalesce(nmm:artistName(nmm:performer(?track)), \"%1\") AS ?artist\n"
                              "       nmm:artistName(nmm:performer(?track)) AS ?rawArtist\n"
                              "       tracker:coalesce(nie:title(nmm:musicAlbum(?track)), \"%2\") AS ?album\n"
                              "       nie:title(nmm:musicAlbum(?track)) AS ?rawAlbum\n"
                              "       nfo:duration(?track) AS ?duration\n"
                              "WHERE {\n"
                              "    ?track nie:url \"%3\".\n"
                              "}")
            .arg(tr("Unknown artist"))
            .arg(tr("Unknown album"))
            .arg(Utils::encodeUrl(trackUrl));
    }

    QStringList PlaylistUtils::unboxTracks(const QVariant& tracksVariant)
    {
        if (tracksVariant.type() == QVariant::String) {
            return QStringList({tracksVariant.toString()});
        }

        if (tracksVariant.type() == QVariant::List) {
            QStringList tracks;
            for (const QVariant& trackVariant : tracksVariant.toList()) {
                tracks.append(trackVariant.toMap().value(QStringLiteral("url")).toString());
            }
            return tracks;
        }

        return QStringList();
    }

    void PlaylistUtils::setPlaylistTracksCount(QString playlistUrl, int tracksCount)
    {
        playlistUrl = QUrl(playlistUrl).toEncoded();

        QSparqlResult* result = mSparqlConnection->syncExec(QSparqlQuery(QStringLiteral("DELETE {\n"
                                                                                        "    ?playlist nfo:entryCounter ?tracksCount\n"
                                                                                        "} WHERE {\n"
                                                                                        "    ?playlist nie:url \"%1\";\n"
                                                                                        "              nfo:entryCounter ?tracksCount.\n"
                                                                                        "}\n")
                                                                             .arg(playlistUrl),
                                                                         QSparqlQuery::DeleteStatement));
        result->deleteLater();

        result = mSparqlConnection->exec(QSparqlQuery(QStringLiteral("INSERT {\n"
                                                                     "    ?playlist nfo:entryCounter %1\n"
                                                                     "} WHERE {\n"
                                                                     "    ?playlist nie:url \"%2\".\n"
                                                                     "}\n")
                                                          .arg(tracksCount)
                                                          .arg(playlistUrl),
                                                      QSparqlQuery::InsertStatement));

        connect(result, &QSparqlResult::finished, result, &QObject::deleteLater);
    }

    void PlaylistUtils::savePlaylist(const QString& playlistUrl, const QStringList& tracks)
    {
        QFile file(QUrl(playlistUrl).path());
        if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
            return;
        }

        int tracksCount = tracks.size();

        file.write(QByteArrayLiteral("[playlist]\nNumberOfEntries=") + QByteArray::number(tracksCount));

        for (int i = 0; i < tracksCount; i++) {
            file.write(QStringLiteral("\nFile%1=%2").arg(i + 1).arg(tracks.at(i)).toUtf8());
        }

        file.close();
    }

    void PlaylistUtils::onTrackerGraphUpdated(const QString& className)
    {
        if (className == "http://www.tracker-project.org/temp/nmm#Playlist") {
            emit playlistsChanged();

            if (mNewPlaylist) {
                setPlaylistTracksCount(mNewPlaylistUrl, mNewPlaylistTracksCount);
                mNewPlaylist = false;
            }
        }
    }
}
