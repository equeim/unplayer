#ifndef UNPLAYER_PLAYLISTUTILS_H
#define UNPLAYER_PLAYLISTUTILS_H

#include <QObject>

class QSparqlConnection;

namespace Unplayer
{

class PlaylistUtils : public QObject
{
    Q_OBJECT
public:
    PlaylistUtils();

    Q_INVOKABLE void newPlaylist(const QString &name, const QVariantList &tracks);
    Q_INVOKABLE void addTracksToPlaylist(const QString &playlistUrl, const QVariantList &newTracks);
    Q_INVOKABLE void removeTrackFromPlaylist(const QString &playlistUrl, int trackIndex);
    Q_INVOKABLE void clearPlaylist(const QString &url);
    Q_INVOKABLE static void removePlaylist(const QString &url);
    static QStringList parsePlaylist(const QString &playlistUrl);
private:
    void setPlaylistTracksCount(const QString &playlistUrl, int tracksCount);
    static void savePlaylist(const QString &playlistUrl, const QStringList &tracks);
private slots:
    void onTrackerGraphUpdated(QString className);
private:
    QSparqlConnection *m_sparqlConnection;

    bool m_newPlaylist;
    QString m_newPlaylistUrl;
    int m_newPlaylistTracksCount;
signals:
    void playlistsChanged();
};

}

#endif // UNPLAYER_PLAYLISTUTILS_H
