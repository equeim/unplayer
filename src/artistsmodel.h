/*
 * Unplayer
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
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

#include "databasemodel.h"

namespace unplayer
{
    class ArtistsModel : public DatabaseModel
    {
        Q_OBJECT
        Q_ENUMS(Role)
        Q_PROPERTY(bool sortDescending READ sortDescending NOTIFY sortDescendingChanged)
    public:
        enum Role
        {
            ArtistRole = Qt::UserRole,
            DisplayedArtistRole,
            AlbumsCountRole,
            TracksCountRole,
            DurationRole
        };

        ArtistsModel();

        QVariant data(const QModelIndex& index, int role) const override;

        bool sortDescending() const;

        Q_INVOKABLE void toggleSortOrder();

        Q_INVOKABLE QStringList getTracksForArtist(int index) const;
        Q_INVOKABLE QStringList getTracksForArtists(const std::vector<int>& indexes) const;
    protected:
        QHash<int, QByteArray> roleNames() const override;

    private:
        void setQuery();

        bool mSortDescending;

    signals:
        void sortDescendingChanged();
    };
}

#endif // UNPLAYER_ARTISTSMODEL_H
