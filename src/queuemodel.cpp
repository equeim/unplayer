/*
 * Unplayer
 * Copyright (C) 2015 Alexey Rochev <equeim@gmail.com>
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

#include "queuemodel.h"
#include "queuemodel.moc"

#include "queue.h"

namespace Unplayer
{

enum QueueModelRole
{
    TitleRole = Qt::UserRole,
    DurationRole,
    ArtistRole,
    AlbumRole
};

void QueueModel::classBegin()
{

}

void QueueModel::componentComplete()
{
    connect(m_queue, &Queue::tracksRemoved, this, &QueueModel::removeTracks);
}

QVariant QueueModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const QueueTrack *track = m_queue->tracks().at(index.row());

    switch (role) {
    case TitleRole:
        return track->title;
    case DurationRole:
        return track->duration;
    case ArtistRole:
        return track->artist;
    case AlbumRole:
        return track->album;
    default:
        return QVariant();
    }
}

int QueueModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_queue->tracks().length();
}

Queue* QueueModel::queue() const
{
    return m_queue;
}

void QueueModel::setQueue(Queue *newQueue)
{
    m_queue = newQueue;
}

QVariantMap QueueModel::get(int trackIndex) const
{
    QVariantMap map;
    map.insert("url", m_queue->tracks().at(trackIndex)->url);
    return map;
}

QHash<int, QByteArray> QueueModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles.insert(TitleRole, "title");
    roles.insert(DurationRole, "duration");
    roles.insert(ArtistRole, "artist");
    roles.insert(AlbumRole, "album");

    return roles;
}

void QueueModel::removeTracks(QList<int> trackIndexes)
{
    for (int i = 0, indexesCount = trackIndexes.size(); i < indexesCount; i++) {
        int trackIndex = trackIndexes.at(i) - i;
        beginRemoveRows(QModelIndex(), trackIndex, trackIndex);
        endRemoveRows();
    }
}

}
