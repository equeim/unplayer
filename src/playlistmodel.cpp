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

#include "playlistmodel.h"
#include "playlistmodel.moc"

#include <QUrl>

#include <QSparqlConnection>
#include <QSparqlQuery>
#include <QSparqlQueryOptions>
#include <QSparqlResult>

#include "playlistutils.h"
#include "utils.h"

namespace Unplayer
{

struct PlaylistTrack
{
    PlaylistTrack(const QString &url)
        : url(url),
          unknownArtist(false),
          unknownAlbum(false)
    {

    }

    QString title;
    QString url;
    qint64 duration;
    QString artist;
    bool unknownArtist;
    QString album;
    bool unknownAlbum;
};

enum PlaylistModelRole
{
    TitleRole = Qt::UserRole,
    UrlRole,
    DurationRole,
    ArtistRole,
    AlbumRole
};

PlaylistModel::PlaylistModel()
    : m_rowCount(0),
      m_loadedTracks(0),
      m_loaded(false)
{

}

PlaylistModel::~PlaylistModel()
{
    qDeleteAll(m_queries);
    qDeleteAll(m_tracks);
}

void PlaylistModel::classBegin()
{

}

void PlaylistModel::componentComplete()
{
    QStringList tracksList = PlaylistUtils::parsePlaylist(m_url);

    if (tracksList.isEmpty()) {
        m_loaded = true;
        emit loadedChanged();
    } else {
        QSparqlConnection *connection = new QSparqlConnection("QTRACKER_DIRECT", QSparqlConnectionOptions(), this);

        for (int i = 0, tracksCount = tracksList.size(); i < tracksCount; i++) {
            QString url = QUrl(tracksList.at(i)).toEncoded();
            m_tracks.append(new PlaylistTrack(url));

            QSparqlResult *result = connection->exec(QSparqlQuery(Utils::singleTrackSparqlQuery(url),
                                                                  QSparqlQuery::SelectStatement));
            result->setProperty("trackIndex", i);
            connect(result, &QSparqlResult::finished, this, &PlaylistModel::onQueryFinished);
            m_queries.append(result);
        }
    }
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const PlaylistTrack *track = m_tracks.at(index.row());

    switch (role) {
    case TitleRole:
        return track->title;
    case UrlRole:
        return track->url;
    case DurationRole:
        return track->duration;
    case ArtistRole:
        return track->artist;
    case AlbumRole:
        return track->album;
    default:
        return QVariant();
    }
}

int PlaylistModel::rowCount(const QModelIndex&) const
{
    return m_rowCount;
}

QString PlaylistModel::url() const
{
    return m_url;
}

void PlaylistModel::setUrl(const QString &newUrl)
{
    m_url = newUrl;
}

bool PlaylistModel::isLoaded() const
{
    return m_loaded;
}

QVariantMap PlaylistModel::get(int trackIndex) const
{
    QVariantMap map;

    const PlaylistTrack *track = m_tracks.at(trackIndex);

    map.insert("title", track->title);
    map.insert("url", track->url);
    map.insert("duration", track->duration);

    map.insert("artist", track->artist);
    if (track->unknownArtist)
        map.insert("rawArtist", QVariant());
    else
        map.insert("rawArtist", track->artist);

    map.insert("album", track->album);
    if (track->unknownAlbum)
        map.insert("rawAlbum", QVariant());
    else
        map.insert("rawAlbum", track->album);

    return map;
}

void PlaylistModel::removeAtIndexes(const QList<int> &trackIndexes)
{
    for (int i = 0, indexesCount = trackIndexes.size(); i < indexesCount; i++) {
        int trackIndex = trackIndexes.at(i) - i;
        beginRemoveRows(QModelIndex(), trackIndex, trackIndex);
        delete m_tracks.takeAt(trackIndex);
        m_rowCount--;
        endRemoveRows();
    }
}

QHash<int, QByteArray> PlaylistModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles.insert(TitleRole, "title");
    roles.insert(UrlRole, "url");
    roles.insert(DurationRole, "duration");
    roles.insert(ArtistRole, "artist");
    roles.insert(AlbumRole, "album");

    return roles;
}

void PlaylistModel::onQueryFinished()
{
    QSparqlResult *result = static_cast<QSparqlResult*>(sender());

    if (result->size() > 0) {
        result->next();
        QSparqlResultRow row = result->current();

        PlaylistTrack *track = m_tracks.at(result->property("trackIndex").toInt());

        track->title = row.value("title").toString();
        track->duration = row.value("duration").toLongLong();

        QVariant artist = row.value("artist");
        if (artist.isValid()) {
            track->artist = artist.toString();
        } else {
            track->artist = tr("Unknown artist");
            track->unknownArtist = true;
        }

        QVariant album = row.value("album");
        if (album.isValid()) {
            track->album = album.toString();
        } else {
            track->album = tr("Unknown album");
            track->unknownAlbum = true;
        }
    }

    m_loadedTracks++;

    if (m_loadedTracks == m_tracks.size()) {
        m_loaded = true;
        emit loadedChanged();

        beginResetModel();
        m_rowCount = m_tracks.size();
        endResetModel();

        qDeleteAll(m_queries);
        m_queries.clear();
    }
}

}
