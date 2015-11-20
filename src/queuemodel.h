#ifndef UNPLAYER_QUEUEMODEL_H
#define UNPLAYER_QUEUEMODEL_H

#include <QAbstractListModel>
#include <QQmlParserStatus>

namespace Unplayer
{

class Queue;

class QueueModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(Queue* queue READ queue WRITE setQueue)
public:
    void classBegin();
    void componentComplete();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    Queue* queue() const;
    void setQueue(Queue *newQueue);
protected:
    QHash<int, QByteArray> roleNames() const;
private:
    void removeTrack(int trackIndex);
private:
    Queue *m_queue;
};

}

#endif // UNPLAYER_QUEUEMODEL_H
