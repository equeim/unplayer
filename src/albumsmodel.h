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

#ifndef UNPLAYER_ALBUMSMODEL_H
#define UNPLAYER_ALBUMSMODEL_H

#include <QQmlParserStatus>
#include "databasemodel.h"

namespace unplayer
{
    class AlbumsModel : public DatabaseModel, public QQmlParserStatus
    {
        Q_OBJECT
        Q_INTERFACES(QQmlParserStatus)
        Q_ENUMS(Role)
        Q_PROPERTY(bool allArtists READ allArtists WRITE setAllArtists)
        Q_PROPERTY(QString artist READ artist WRITE setArtist)
    public:
        enum Role
        {
            ArtistRole = Qt::UserRole,
            DisplayedArtistRole,
            UnknownArtistRole,
            AlbumRole,
            DisplayedAlbumRole,
            UnknownAlbumRole,
            TracksCountRole,
            DurationRole
        };

        AlbumsModel();
        void classBegin() override;
        void componentComplete() override;

        QVariant data(const QModelIndex& index, int role) const override;

        bool allArtists() const;
        void setAllArtists(bool allArtists);

        const QString& artist() const;
        void setArtist(const QString& artist);

        Q_INVOKABLE QStringList getTracksForAlbum(int index) const;
        Q_INVOKABLE QStringList getTracksForAlbums(const QVector<int>& indexes) const;

    protected:
        QHash<int, QByteArray> roleNames() const override;

    private:
        bool mAllArtists;
        QString mArtist;
    };
}

#endif // UNPLAYER_ALBUMSMODEL_H
