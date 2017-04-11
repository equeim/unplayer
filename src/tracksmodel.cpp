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

#include "tracksmodel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QSqlDriver>
#include <QSqlError>
#include <QUrl>

namespace unplayer
{
    namespace
    {
        enum Field
        {
            FilePathField,
            TitleField,
            ArtistField,
            AlbumField,
            DurationField
        };
    }

    TracksModel::TracksModel()
        : mAllArtists(true),
          mAllAlbums(true)
    {

    }

    void TracksModel::componentComplete()
    {
        QString query(QLatin1String("SELECT filePath, title, artist, album, duration FROM tracks "));

        if (mAllArtists) {
            if (!mGenre.isEmpty()) {
                query += QLatin1String("WHERE genre = ? ");
            }
            query += QLatin1String("GROUP BY id, artist, album "
                                   "ORDER BY artist = '', artist, album = '', year, album, trackNumber, title");
            mQuery->prepare(query);
            if (!mGenre.isEmpty()) {
                mQuery->addBindValue(mGenre);
            }
        } else {
            query += QLatin1String("WHERE artist = ? ");
            if (mAllAlbums) {
                query += QLatin1String("GROUP BY id, album "
                                       "ORDER BY album = '', year, album, trackNumber, title ");
                mQuery->prepare(query);
                mQuery->addBindValue(mArtist);
            } else {
                query += QLatin1String("AND album = ? "
                                       "GROUP BY id "
                                       "ORDER BY trackNumber, title");
                mQuery->prepare(query);
                mQuery->addBindValue(mArtist);
                mQuery->addBindValue(mAlbum);
            }
        }

        execQuery();
    }

    QVariant TracksModel::data(const QModelIndex& index, int role) const
    {
        mQuery->seek(index.row());

        switch (role) {
        case FilePathRole:
            return mQuery->value(FilePathField);
        case TitleRole:
            return mQuery->value(TitleField);
        case ArtistRole:
        {
            const QString artist(mQuery->value(ArtistField).toString());
            if (artist.isEmpty()) {
                return qApp->translate("unplayer", "Unknown artist");
            }
            return artist;
        }
        case AlbumRole:
        {
            const QString album(mQuery->value(AlbumField).toString());
            if (album.isEmpty()) {
                return qApp->translate("unplayer", "Unknown album");
            }
            return album;
        }
        case DurationRole:
            return mQuery->value(DurationField);
        default:
            return QVariant();
        }
    }

    bool TracksModel::allArtists() const
    {
        return mAllArtists;
    }

    void TracksModel::setAllArtists(bool allArtists)
    {
        mAllArtists = allArtists;
    }

    bool TracksModel::allAlbums() const
    {
        return mAllAlbums;
    }

    void TracksModel::setAllAlbums(bool allAlbums)
    {
        mAllAlbums = allAlbums;
    }

    const QString& TracksModel::artist() const
    {
        return mArtist;
    }

    void TracksModel::setArtist(const QString& artist)
    {
        mArtist = artist;
    }

    const QString& TracksModel::album() const
    {
        return mAlbum;
    }

    void TracksModel::setAlbum(const QString& album)
    {
        mAlbum = album;
    }

    const QString& TracksModel::genre() const
    {
        return mGenre;
    }

    void TracksModel::setGenre(const QString& genre)
    {
        mGenre = genre;
    }

    QStringList TracksModel::getTracks(const QVector<int>& indexes)
    {
        QStringList tracks;
        tracks.reserve(indexes.size());
        for (int index : indexes) {
            mQuery->seek(index);
            tracks.append(mQuery->value(FilePathField).toString());
        }
        return tracks;
    }

    QHash<int, QByteArray> TracksModel::roleNames() const
    {
        return {{FilePathRole, "filePath"},
                {TitleRole, "title"},
                {ArtistRole, "artist"},
                {AlbumRole, "album"},
                {DurationRole, "duration"}};
    }
}
