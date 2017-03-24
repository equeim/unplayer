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

#include "playlistmodel.h"

#include <QUrl>

#include <QSparqlConnection>
#include <QSparqlQuery>
#include <QSparqlQueryOptions>
#include <QSparqlResult>

#include "playlistutils.h"

namespace unplayer
{
    namespace
    {
        enum PlaylistModelRole
        {
            UrlRole = Qt::UserRole,
            TitleRole,
            ArtistRole,
            AlbumRole,
            DurationRole
        };
    }

    struct PlaylistTrack
    {
        explicit PlaylistTrack(const QString& url)
            : url(url),
              unknownArtist(false),
              unknownAlbum(false)
        {
        }

        QString url;
        QString title;
        QString artist;
        bool unknownArtist;
        QString album;
        bool unknownAlbum;
        qint64 duration;
    };

    PlaylistModel::PlaylistModel()
        : mRowCount(0),
          mLoadedTracks(0),
          mLoaded(false)
    {
    }

    PlaylistModel::~PlaylistModel()
    {
        qDeleteAll(mQueries);
        qDeleteAll(mTracks);
    }

    void PlaylistModel::classBegin()
    {
    }

    void PlaylistModel::componentComplete()
    {
        QStringList tracksList(PlaylistUtils::parsePlaylist(mUrl));

        if (tracksList.isEmpty()) {
            mLoaded = true;
            emit loadedChanged();
        } else {
            QSparqlConnection* connection = new QSparqlConnection(QStringLiteral("QTRACKER_DIRECT"), QSparqlConnectionOptions(), this);

            for (int i = 0, max = tracksList.size(); i < max; i++) {
                QString url(tracksList.at(i));
                mTracks.append(new PlaylistTrack(url));

                QSparqlResult* result = connection->exec(QSparqlQuery(PlaylistUtils::trackSparqlQuery(url),
                                                                      QSparqlQuery::SelectStatement));
                result->setProperty("trackIndex", i);
                connect(result, &QSparqlResult::finished, this, &PlaylistModel::onQueryFinished);
                mQueries.append(result);
            }
        }
    }

    QVariant PlaylistModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid()) {
            return QVariant();
        }

        const PlaylistTrack* track = mTracks.at(index.row());

        switch (role) {
        case UrlRole:
            return track->url;
        case TitleRole:
            return track->title;
        case ArtistRole:
            return track->artist;
        case AlbumRole:
            return track->album;
        case DurationRole:
            return track->duration;
        default:
            return QVariant();
        }
    }

    int PlaylistModel::rowCount(const QModelIndex&) const
    {
        return mRowCount;
    }

    QString PlaylistModel::url() const
    {
        return mUrl;
    }

    void PlaylistModel::setUrl(const QString& newUrl)
    {
        mUrl = newUrl;
    }

    bool PlaylistModel::isLoaded() const
    {
        return mLoaded;
    }

    QVariantMap PlaylistModel::get(int trackIndex) const
    {
        const PlaylistTrack* track = mTracks.at(trackIndex);

        QVariantMap map{{QStringLiteral("title"), track->title},
                        {QStringLiteral("url"), track->url},
                        {QStringLiteral("artist"), track->artist},
                        {QStringLiteral("album"), track->album},
                        {QStringLiteral("duration"), track->duration}};

        if (!track->unknownArtist) {
            map.insert(QStringLiteral("rawArtist"), track->artist);
        }

        if (!track->unknownAlbum) {
            map.insert(QStringLiteral("rawAlbum"), track->album);
        }

        return map;
    }

    void PlaylistModel::removeAtIndexes(const QList<int>& trackIndexes)
    {
        int indexesCount = trackIndexes.size();

        if (indexesCount == mTracks.size()) {
            beginResetModel();
            qDeleteAll(mTracks);
            mTracks.clear();
            mRowCount = 0;
            endResetModel();
        } else {
            for (int i = 0, max = trackIndexes.size(); i < max; i++) {
                int index = trackIndexes.at(i) - i;
                beginRemoveRows(QModelIndex(), index, index);
                delete mTracks.takeAt(index);
                mRowCount--;
                endRemoveRows();
            }
        }
    }

    QHash<int, QByteArray> PlaylistModel::roleNames() const
    {
        return {{UrlRole, QByteArrayLiteral("url")},
                {TitleRole, QByteArrayLiteral("title")},
                {ArtistRole, QByteArrayLiteral("artist")},
                {AlbumRole, QByteArrayLiteral("album")},
                {DurationRole, QByteArrayLiteral("duration")}};
    }

    void PlaylistModel::onQueryFinished()
    {
        QSparqlResult* result = static_cast<QSparqlResult*>(sender());

        if (result->size() > 0) {
            result->next();
            QSparqlResultRow row(result->current());

            PlaylistTrack* track = mTracks.at(result->property("trackIndex").toInt());

            track->title = row.value(QStringLiteral("title")).toString();
            track->duration = row.value(QStringLiteral("duration")).toLongLong();

            QVariant artist(row.value(QStringLiteral("artist")));
            if (artist.isValid()) {
                track->artist = artist.toString();
            } else {
                track->artist = tr("Unknown artist");
                track->unknownArtist = true;
            }

            QVariant album(row.value(QStringLiteral("album")));
            if (album.isValid()) {
                track->album = album.toString();
            } else {
                track->album = tr("Unknown album");
                track->unknownAlbum = true;
            }
        }

        mLoadedTracks++;

        if (mLoadedTracks == mTracks.size()) {
            mLoaded = true;
            emit loadedChanged();

            beginResetModel();
            mRowCount = mTracks.size();
            endResetModel();

            qDeleteAll(mQueries);
            mQueries.clear();
        }
    }
}
