/*
 * Unplayer
 * Copyright (C) 2015-2019 Alexey Rochev <equeim@gmail.com>
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

#ifndef UNPLAYER_ARTISTSMODEL_H
#define UNPLAYER_ARTISTSMODEL_H

#include <vector>
#include <QAbstractListModel>

#include "abstractlibrarymodel.h"
#include "librarytrack.h"

namespace unplayer
{
    struct Artist
    {
        int id;
        QString artist;
        QString displayedArtist;
        int albumsCount;
        int tracksCount;
        int duration;
    };

    class ArtistsModel : public AbstractLibraryModel<Artist>
    {
        Q_OBJECT
        Q_PROPERTY(bool sortDescending READ sortDescending NOTIFY sortDescendingChanged)
    public:
        enum Role
        {
            ArtistIdRole = Qt::UserRole,
            ArtistRole,
            DisplayedArtistRole,
            AlbumsCountRole,
            TracksCountRole,
            DurationRole
        };
        Q_ENUM(Role)

        ArtistsModel();

        QVariant data(const QModelIndex& index, int role) const override;

        bool sortDescending() const;

        Q_INVOKABLE void toggleSortOrder();

        Q_INVOKABLE std::vector<unplayer::LibraryTrack> getTracksForArtist(int index) const;
        Q_INVOKABLE std::vector<unplayer::LibraryTrack> getTracksForArtists(const std::vector<int>& indexes) const;
        Q_INVOKABLE QStringList getTrackPathsForArtist(int index) const;
        Q_INVOKABLE QStringList getTrackPathsForArtists(const std::vector<int>& indexes) const;

        Q_INVOKABLE void removeArtist(int index, bool deleteFiles);
        Q_INVOKABLE void removeArtists(const std::vector<int>& indexes, bool deleteFiles);
    protected:
        class ItemFactory : public AbstractItemFactory
        {
        public:
            Artist itemFromQuery(const QSqlQuery& query) override;
        };

        QHash<int, QByteArray> roleNames() const override;

        QString makeQueryString() override;
        AbstractItemFactory* createItemFactory() override;

    private:
        std::vector<Artist>& mArtists = mItems;
        bool mSortDescending;

    signals:
        void sortDescendingChanged();
    };
}

#endif // UNPLAYER_ARTISTSMODEL_H
