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

#ifndef UNPLAYER_TRACKSMODEL_H
#define UNPLAYER_TRACKSMODEL_H

#include <QQmlParserStatus>
#include "databasemodel.h"

namespace unplayer
{
    class TracksModelSortMode : public QObject
    {
        Q_OBJECT
        Q_ENUMS(Mode)
    public:
        enum Mode
        {
            Title,
            AddedDate,
            ArtistAlbumTitle,
            ArtistAlbumYear
        };
    };

    class TracksModelInsideAlbumSortMode : public QObject
    {
        Q_OBJECT
        Q_ENUMS(Mode)
    public:
        enum Mode
        {
            Title,
            TrackNumber
        };
    };

    class TracksModel : public DatabaseModel
    {
        Q_OBJECT
        Q_ENUMS(Role)

        Q_PROPERTY(bool allArtists READ allArtists WRITE setAllArtists)
        Q_PROPERTY(bool allAlbums READ allAlbums WRITE setAllAlbums)
        Q_PROPERTY(QString artist READ artist WRITE setArtist)
        Q_PROPERTY(QString album READ album WRITE setAlbum)
        Q_PROPERTY(QString genre READ genre WRITE setGenre)

        Q_PROPERTY(bool sortDescending READ sortDescending WRITE setSortDescending)
        Q_PROPERTY(unplayer::TracksModelSortMode::Mode sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
        Q_PROPERTY(unplayer::TracksModelInsideAlbumSortMode::Mode insideAlbumSortMode READ insideAlbumSortMode WRITE setInsideAlbumSortMode NOTIFY insideAlbumSortModeChanged)
    public:
        enum Role
        {
            FilePathRole = Qt::UserRole,
            TitleRole,
            ArtistRole,
            AlbumRole,
            DurationRole
        };

        using SortMode = TracksModelSortMode::Mode;
        using InsideAlbumSortMode = TracksModelInsideAlbumSortMode::Mode;

        TracksModel();
        ~TracksModel() override;
        void componentComplete() override;

        QVariant data(const QModelIndex& index, int role) const override;

        bool allArtists() const;
        void setAllArtists(bool allArtists);

        bool allAlbums() const;
        void setAllAlbums(bool allAlbums);

        const QString& artist() const;
        void setArtist(const QString& artist);

        const QString& album() const;
        void setAlbum(const QString& album);

        const QString& genre() const;
        void setGenre(const QString& genre);

        bool sortDescending() const;
        void setSortDescending(bool descending);

        SortMode sortMode() const;
        void setSortMode(SortMode mode);

        InsideAlbumSortMode insideAlbumSortMode() const;
        void setInsideAlbumSortMode(InsideAlbumSortMode mode);

        Q_INVOKABLE QStringList getTracks(const QVector<int>& indexes);

    protected:
        QHash<int, QByteArray> roleNames() const override;

    private:
        void setQuery();

        bool mAllArtists;
        bool mAllAlbums;
        QString mArtist;
        QString mAlbum;
        QString mGenre;

        bool mSortDescending;
        SortMode mSortMode;
        InsideAlbumSortMode mInsideAlbumSortMode;

    signals:
        void sortModeChanged();
        void insideAlbumSortModeChanged();
    };
}

#endif // UNPLAYER_TRACKSMODEL_H
