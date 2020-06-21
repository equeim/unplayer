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

#ifndef UNPLAYER_QUEUEMODEL_H
#define UNPLAYER_QUEUEMODEL_H

#include <QAbstractListModel>

namespace unplayer
{
    class Queue;

    class QueueModel : public QAbstractListModel
    {
        Q_OBJECT
        Q_PROPERTY(unplayer::Queue* queue READ queue WRITE setQueue)
    public:
        enum Role
        {
            UrlRole = Qt::UserRole,
            IsLocalFileRole,
            FilePathRole,
            TitleRole,
            ArtistRole,
            DisplayedArtistRole,
            AlbumRole,
            DisplayedAlbumRole,
            DurationRole
        };
        Q_ENUM(Role)

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;

        Queue* queue() const;
        void setQueue(Queue* queue);

    protected:
        QHash<int, QByteArray> roleNames() const override;

    private:
        Queue* mQueue = nullptr;
    };
}

#endif // UNPLAYER_QUEUEMODEL_H
