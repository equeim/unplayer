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

#ifndef UNPLAYER_PLAYLISTSMODEL_H
#define UNPLAYER_PLAYLISTSMODEL_H

#include <vector>
#include <QAbstractListModel>

namespace unplayer
{
    struct PlaylistsModelItem
    {
        QString filePath;
        QString name;
        int tracksCount;

        bool operator==(const PlaylistsModelItem& other) const;
    };

    class PlaylistsModel : public QAbstractListModel
    {
        Q_OBJECT
    public:
        enum Role
        {
            FilePathRole = Qt::UserRole,
            NameRole,
            TracksCountRole
        };
        Q_ENUM(Role)

        PlaylistsModel();

        QVariant data(const QModelIndex& index, int role) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;

        Q_INVOKABLE void removePlaylists(const std::vector<int>& indexes) const;

        Q_INVOKABLE QStringList getTracksForPlaylists(const std::vector<int>& indexes) const;
    protected:
        QHash<int, QByteArray> roleNames() const override;

    private:
        void update();

        std::vector<PlaylistsModelItem> mPlaylists;
    };
}

Q_DECLARE_TYPEINFO(unplayer::PlaylistsModelItem, Q_MOVABLE_TYPE);

#endif // UNPLAYER_PLAYLISTSMODEL_H
