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

#ifndef UNPLAYER_QUEUEMODEL_H
#define UNPLAYER_QUEUEMODEL_H

#include <memory>
#include <QAbstractListModel>

#include "queue.h"

namespace unplayer
{
    class QueueModel : public QAbstractListModel
    {
        Q_OBJECT
        Q_ENUMS(Role)
        Q_PROPERTY(unplayer::Queue* queue READ queue WRITE setQueue)
    public:
        enum Role
        {
            FilePathRole = Qt::UserRole,
            TitleRole,
            ArtistRole,
            DisplayedArtistRole,
            AlbumRole,
            DisplayedAlbumRole,
            DurationRole
        };

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;

        Queue* queue() const;
        void setQueue(Queue* queue);

        Q_INVOKABLE QStringList getTracks(const QVector<int>& indexes);

    protected:
        QHash<int, QByteArray> roleNames() const override;

    private:
        Queue* mQueue = nullptr;
        QList<std::shared_ptr<QueueTrack>> mTracks;
    };
}

#endif // UNPLAYER_QUEUEMODEL_H
