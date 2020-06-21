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

#include "queuemodel.h"

#include "queue.h"

namespace unplayer
{
    QVariant QueueModel::data(const QModelIndex& index, int role) const
    {
        if (!mQueue || !index.isValid()) {
            return QVariant();
        }

        const QueueTrack* track = mQueue->tracks()[static_cast<size_t>(index.row())].get();

        switch (role) {
        case UrlRole:
            return track->url;
        case IsLocalFileRole:
            return track->url.isLocalFile();
        case FilePathRole:
            return track->url.path();
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
        return mQueue ? static_cast<int>(mQueue->tracks().size()) : 0;
    }

    Queue* QueueModel::queue() const
    {
        return mQueue;
    }

    void QueueModel::setQueue(Queue* queue)
    {
        if (queue == mQueue || !queue) {
            return;
        }

        mQueue = queue;

        QObject::connect(mQueue, &Queue::tracksAboutToBeAdded, this, [=](int count) {
            const int first = rowCount();
            beginInsertRows(QModelIndex(), first, first + count - 1);
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
        return {{UrlRole, "url"},
                {IsLocalFileRole, "isLocalFile"},
                {FilePathRole, "filePath"},
                {TitleRole, "title"},
                {ArtistRole, "artist"},
                {DisplayedArtistRole, "displayedArtist"},
                {AlbumRole, "album"},
                {DisplayedAlbumRole, "displayedAlbum"},
                {DurationRole, "duration"}};
    }
}
