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
    AlbumRole,
    MediaArtRole
};

void QueueModel::classBegin()
{

}

void QueueModel::componentComplete()
{
    connect(m_queue, &Queue::trackRemoved, this, &QueueModel::removeTrack);
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
    case MediaArtRole:
        return track->mediaArt;
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

QHash<int, QByteArray> QueueModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles.insert(TitleRole, "title");
    roles.insert(DurationRole, "duration");
    roles.insert(ArtistRole, "artist");
    roles.insert(AlbumRole, "album");
    roles.insert(MediaArtRole, "mediaArt");

    return roles;
}

void QueueModel::removeTrack(int trackIndex)
{
    beginRemoveRows(QModelIndex(), trackIndex, trackIndex);
    endRemoveRows();
}

}
