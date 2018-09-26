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

#include "queuemodel.h"

namespace unplayer
{
    QVariant QueueModel::data(const QModelIndex& index, int role) const
    {
        if (!mQueue || !index.isValid()) {
            return QVariant();
        }

        const QueueTrack* track = mQueue->tracks()[index.row()].get();

        switch (role) {
        case FilePathRole:
            return track->filePath;
        case TitleRole:
            return track->title;
        case ArtistRole:
            return track->artist;
        case DisplayedArtistRole:
            return track->artist;
        case AlbumRole:
            return track->album;
        case DisplayedAlbumRole:
            return track->album;
        case DurationRole:
            return track->duration;
        default:
            return QVariant();
        }
    }

    int QueueModel::rowCount(const QModelIndex&) const
    {
        if (mQueue) {
            return mQueue->tracks().size();
        }
        return 0;
    }

    Queue* QueueModel::queue() const
    {
        return mQueue;
    }

    void QueueModel::setQueue(Queue* queue)
    {
        mQueue = queue;

        QObject::connect(mQueue, &Queue::tracksAboutToBeAdded, this, [=](int count) {
            const int fisrt = rowCount();
            beginInsertRows(QModelIndex(), fisrt, fisrt + count);
        });

        QObject::connect(mQueue, &Queue::tracksAdded, this, [=]() {
            endInsertRows();
        });

        QObject::connect(mQueue, &Queue::trackAboutToBeRemoved, this, [=](int index) {
            beginRemoveRows(QModelIndex(), index, index);
        });

        QObject::connect(mQueue, &Queue::trackRemoved, this, [=]() {
            endRemoveRows();
        });

        QObject::connect(mQueue, &Queue::aboutToBeCleared, this, [=]() {
            beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
        });

        QObject::connect(mQueue, &Queue::cleared, this, [=]() {
            endRemoveRows();
        });
    }

    QHash<int, QByteArray> QueueModel::roleNames() const
    {
        return {{FilePathRole, "filePath"},
                {TitleRole, "title"},
                {ArtistRole, "artist"},
                {DisplayedArtistRole, "displayedArtist"},
                {AlbumRole, "album"},
                {DisplayedAlbumRole, "displayedAlbum"},
                {DurationRole, "duration"}};
    }
}
