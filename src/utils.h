#ifndef UNPLAYER_UTILS_H
#define UNPLAYER_UTILS_H

#include <QObject>
#include <QUrl>

class QQmlEngine;
class QSparqlConnection;

namespace Unplayer
{

class Utils : public QObject
{
    Q_OBJECT
public:
    Utils();

    Q_INVOKABLE void newPlaylist(const QString &name, const QStringList &tracks);
    Q_INVOKABLE void addTracksToPlaylist(const QString &playlistUrl, const QStringList &newTracks);
    Q_INVOKABLE void removeTrackFromPlaylist(const QString &playlistUrl, int trackIndex);
    Q_INVOKABLE void clearPlaylist(const QString &url);
    Q_INVOKABLE static void removePlaylist(const QString &url);
    static QStringList parsePlaylist(const QString &playlistUrl);

    Q_INVOKABLE static QString mediaArt(const QString &artistName, const QString &albumTitle);
    Q_INVOKABLE static QString mediaArtForArtist(const QString &artistName);
    Q_INVOKABLE static QString randomMediaArt();

    Q_INVOKABLE static QString formatDuration(uint seconds);

    Q_INVOKABLE static QString escapeRegExp(const QString &string);
    Q_INVOKABLE static QString escapeSparql(QString string);
private:
    void setPlaylistTracksCount(const QString &playlistUrl, int tracksCount);
    static void savePlaylist(const QString &playlistUrl, const QStringList &tracks);
    static QString mediaArtMd5(QString string);
private slots:
    void onTrackerGraphUpdated(QString className);
private:
    QSparqlConnection *m_sparqlConnection;

    bool m_newPlaylist;
    QString m_newPlaylistUrl;
    int m_newPlaylistTracksCount;

    static const QString m_mediaArtDirectoryPath;
signals:
    void playlistsChanged();
};

}

#endif // UNPLAYER_UTILS_H
