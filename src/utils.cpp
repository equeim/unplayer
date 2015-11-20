#include "utils.h"
#include "utils.moc"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDBusConnection>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>

#include <QSparqlConnection>
#include <QSparqlConnectionOptions>
#include <QSparqlQuery>
#include <QSparqlResult>

namespace Unplayer
{

const QString Utils::m_mediaArtDirectoryPath = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + "/media-art";

Utils::Utils()
    : m_sparqlConnection(new QSparqlConnection("QTRACKER_DIRECT", QSparqlConnectionOptions(), this)),
      m_newPlaylist(false)
{
    qsrand(QDateTime::currentMSecsSinceEpoch());

    QDBusConnection::sessionBus().connect("org.freedesktop.Tracker1",
                                          "/org/freedesktop/Tracker1/Resources",
                                          "org.freedesktop.Tracker1.Resources",
                                          "GraphUpdated",
                                          this,
                                          SLOT(onTrackerGraphUpdated(QString)));
}

void Utils::newPlaylist(const QString &name, const QStringList &tracks)
{
    m_newPlaylist = true;
    m_newPlaylistUrl = QUrl("file://" + QStandardPaths::writableLocation(QStandardPaths::MusicLocation) + "/playlists/" + name + ".pls").toEncoded();
    m_newPlaylistTracksCount = tracks.size();
    savePlaylist(m_newPlaylistUrl, tracks);
}

void Utils::addTracksToPlaylist(const QString &playlistUrl, const QStringList &newTracks)
{
    QStringList tracks = parsePlaylist(playlistUrl);
    tracks.append(newTracks);
    savePlaylist(playlistUrl, tracks);
    setPlaylistTracksCount(playlistUrl, tracks.size());
}

void Utils::removeTrackFromPlaylist(const QString &playlistUrl, int trackIndex)
{
    QStringList tracks = parsePlaylist(playlistUrl);
    tracks.removeAt(trackIndex);
    savePlaylist(playlistUrl, tracks);
    setPlaylistTracksCount(playlistUrl, tracks.size());
}

void Utils::clearPlaylist(const QString &url)
{
    savePlaylist(url, QStringList());
    setPlaylistTracksCount(url, 0);
}

void Utils::removePlaylist(const QString &url)
{
    QFile(QUrl(url).path()).remove();
}

QStringList Utils::parsePlaylist(const QString &playlistUrl)
{
    QFile file(QUrl(playlistUrl).path());
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray line = file.readLine();
        if (line.trimmed() == "[playlist]") {
            QStringList tracks;
            while (!file.atEnd()) {
                line = file.readLine();
                if (line.startsWith("File")) {
                    int index = line.indexOf('=');
                    if (index >= 0) {
                        QByteArray url = line.mid(index + 1).trimmed();
                        if (!url.isEmpty())
                            tracks.append(url);
                    }
                }
            }
            return tracks;
        }
    }

    return QStringList();
}

QString Utils::mediaArt(const QString &artistName, const QString &albumTitle)
{
    if (artistName.isEmpty() || albumTitle.isEmpty())
        return QString();

    QString filePath = m_mediaArtDirectoryPath +
            QDir::separator() +
            "album-" +
            mediaArtMd5(artistName) +
            "-" +
            mediaArtMd5(albumTitle) +
            ".jpeg";

    if (QFile(filePath).exists())
        return filePath;

    return QString();
}

QString Utils::mediaArtForArtist(const QString &artistName)
{
    QDir mediaArtDirectory(m_mediaArtDirectoryPath);
    QStringList nameFilters;
    nameFilters.append("album-" +
                       mediaArtMd5(artistName) +
                       "-*.jpeg");

    QStringList mediaArtList = mediaArtDirectory.entryList(nameFilters);

    if (mediaArtList.length() == 0)
        return QString();

    static int random = qrand();
    return mediaArtDirectory.filePath(mediaArtList.at(random % mediaArtList.length()));
}

QString Utils::randomMediaArt()
{
    QDir mediaArtDirectory(m_mediaArtDirectoryPath);
    QStringList mediaArtList = mediaArtDirectory.entryList(QDir::Files);

    if (mediaArtList.length() == 0)
        return QString();

    return mediaArtDirectory.filePath(mediaArtList.at(qrand() % mediaArtList.length()));
}

QString Utils::formatDuration(uint seconds)
{
    int hours = seconds / 3600;
    seconds %= 3600;
    int minutes = seconds / 60;
    seconds %= 60;

    QString etaString;

    if (hours > 0)
        etaString +=  tr("%1 h ").arg(hours);

    if (minutes > 0)
        etaString +=  tr("%1 m ").arg(minutes);

    if (hours == 0 &&
            (seconds > 0 ||
             minutes == 0))
        etaString +=  tr("%1 s").arg(seconds);

    return etaString;
}

QString Utils::escapeRegExp(const QString &string)
{
    return QRegularExpression::escape(string);
}

QString Utils::escapeSparql(QString string)
{
    return string.
            replace("\t", "\\t").
            replace("\n", "\\n").
            replace("\r", "\\r").
            replace("\b", "\\b").
            replace("\f", "\\f").
            replace("\"", "\\\"").
            replace("'", "\\'").
            replace("\\", "\\\\");
}

void Utils::setPlaylistTracksCount(const QString &playlistUrl, int tracksCount)
{
    QSparqlResult *result = m_sparqlConnection->syncExec(QSparqlQuery(QString("DELETE {\n"
                                             "    ?playlist nfo:entryCounter ?tracksCount\n"
                                             "} WHERE {\n"
                                             "    ?playlist nie:url \"%1\";\n"
                                             "              nfo:entryCounter ?tracksCount.\n"
                                             "}\n").arg(playlistUrl),
                                     QSparqlQuery::DeleteStatement));
    result->deleteLater();

    result = m_sparqlConnection->exec(QSparqlQuery(QString("INSERT {\n"
                                             "    ?playlist nfo:entryCounter %1\n"
                                             "} WHERE {\n"
                                             "    ?playlist nie:url \"%2\".\n"
                                             "}\n").arg(tracksCount).arg(playlistUrl),
                                     QSparqlQuery::InsertStatement));

    connect(result, &QSparqlResult::finished, result, &QObject::deleteLater);
}

void Utils::savePlaylist(const QString &playlistUrl, const QStringList &tracks)
{
    QFile file(QUrl(playlistUrl).path());
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate))
        return;

    int tracksCount = tracks.size();

    file.write(QByteArray("[playlist]\nNumberOfEntries=") + QByteArray::number(tracksCount));

    for (int i = 0; i < tracksCount; i++) {
        file.write(QByteArray("\nFile") +
                   QByteArray::number(i + 1) +
                   "=" +
                   tracks.at(i).toUtf8());
    }

    file.close();
}

QString Utils::mediaArtMd5(QString string)
{
    string = string.
            replace(QRegularExpression("\\([^\\)]*\\)"), QString()).
            replace(QRegularExpression("\\{[^\\}]*\\}"), QString()).
            replace(QRegularExpression("\\[[^\\]]*\\]"), QString()).
            replace(QRegularExpression("<[^>]*>"), QString()).
            replace(QRegularExpression("[\\(\\)_\\{\\}\\[\\]!@#\\$\\^&\\*\\+=|/\\\'\"?<>~`]"), QString()).
            trimmed().
            replace("\t", " ").
            replace(QRegularExpression("  +"), " ").
            normalized(QString::NormalizationForm_KD).
            toLower();

    if (string.isEmpty())
        string = " ";

    return QCryptographicHash::hash(string.toUtf8(), QCryptographicHash::Md5).toHex();
}

void Utils::onTrackerGraphUpdated(QString className)
{
    if (className == "http://www.tracker-project.org/temp/nmm#Playlist") {
        emit playlistsChanged();

        if (m_newPlaylist) {
            setPlaylistTracksCount(m_newPlaylistUrl, m_newPlaylistTracksCount);
            m_newPlaylist = false;
        }
    }
}

}
