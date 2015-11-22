#include "utils.h"
#include "utils.moc"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>

namespace Unplayer
{

const QString Utils::m_mediaArtDirectoryPath = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + "/media-art";

Utils::Utils()
{
    qsrand(QDateTime::currentMSecsSinceEpoch());
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

}
