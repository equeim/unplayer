#ifndef UNPLAYER_PLAYLISTMODEL_H
#define UNPLAYER_PLAYLISTMODEL_H

#include <QAbstractListModel>
#include <QQmlParserStatus>

class QSparqlResult;

namespace Unplayer
{

struct PlaylistTrack;

class PlaylistModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QString url READ url WRITE setUrl)
    Q_PROPERTY(bool loaded READ isLoaded NOTIFY loadedChanged)
public:
    PlaylistModel();
    ~PlaylistModel();
    void classBegin();
    void componentComplete();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex&) const;

    QString url() const;
    void setUrl(const QString &newUrl);

    bool isLoaded() const;

    Q_INVOKABLE QVariantMap get(int trackIndex) const;
    Q_INVOKABLE void removeAt(int trackIndex);
    Q_INVOKABLE void clear();
protected:
    QHash<int, QByteArray> roleNames() const;
private:
    void onQueryFinished();
private:
    QList<PlaylistTrack*> m_tracks;
    int m_rowCount;

    QList<QSparqlResult*> m_queries;
    QHash<QString, PlaylistTrack*> m_uniqueTracksHash;
    int m_loadedTracks;

    QString m_url;
    bool m_loaded;
signals:
    void loadedChanged();
};

}

#endif // UNPLAYER_PLAYLISTMODEL_H
