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
    class TracksModel : public DatabaseModel, public QQmlParserStatus
    {
        Q_OBJECT
        Q_INTERFACES(QQmlParserStatus)
        Q_ENUMS(Role)
        Q_PROPERTY(bool allArtists READ allArtists WRITE setAllArtists)
        Q_PROPERTY(bool allAlbums READ allAlbums WRITE setAllAlbums)
        Q_PROPERTY(QString artist READ artist WRITE setArtist)
        Q_PROPERTY(QString album READ album WRITE setAlbum)
        Q_PROPERTY(QString genre READ genre WRITE setGenre)
    public:
        enum Role
        {
            FilePathRole = Qt::UserRole,
            TitleRole,
            ArtistRole,
            AlbumRole,
            DurationRole
        };

        TracksModel();
        void componentComplete() override;
        void classBegin() override;

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

        Q_INVOKABLE QStringList getTracks(const QVector<int>& indexes);

    protected:
        QHash<int, QByteArray> roleNames() const override;

    private:
        bool mAllArtists;
        bool mAllAlbums;
        QString mArtist;
        QString mAlbum;
        QString mGenre;
    };
}

#endif // UNPLAYER_TRACKSMODEL_H
