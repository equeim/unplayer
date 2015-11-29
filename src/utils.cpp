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

#include "utils.h"
#include "utils.moc"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>

namespace Unplayer
{

const QString Utils::m_mediaArtDirectoryPath = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + "/media-art";

Utils::Utils()
{
    qsrand(QDateTime::currentMSecsSinceEpoch());
}

QString Utils::mediaArt(const QString &artist, const QString &album, const QString &trackUrl)
{
    if (!artist.isEmpty() && !album.isEmpty()) {
        QString filePath = m_mediaArtDirectoryPath +
                QDir::separator() +
                "album-" +
                mediaArtMd5(artist) +
                "-" +
                mediaArtMd5(album) +
                ".jpeg";

        if (QFileInfo(filePath).exists())
            return filePath;
    }

    if (trackUrl.isEmpty())
        return QString();

    QString trackDirectory = QFileInfo(QUrl(trackUrl).path()).path();

    QStringList foundMediaArt = QDir(trackDirectory).
            entryList(QDir::Files).
            filter(QRegularExpression("^(albumart.*|cover|folder|front)\\.(jpeg|jpg|png)$",
                                      QRegularExpression::CaseInsensitiveOption));

    if (foundMediaArt.isEmpty())
        return QString();

    return trackDirectory + QDir::separator() + foundMediaArt.first();
}

QString Utils::mediaArtForArtist(const QString &artist)
{
    if (artist.isEmpty())
        return QString();

    QFileInfoList mediaArtList = QDir(m_mediaArtDirectoryPath)
            .entryInfoList(QStringList() << "album-" + mediaArtMd5(artist) + "-?*.jpeg",
                           QDir::Files);

    if (mediaArtList.isEmpty())
        return QString();

    static int random = qrand();
    return mediaArtList.at(random % mediaArtList.size()).filePath();
}

QString Utils::randomMediaArt()
{
    QFileInfoList mediaArtList = QDir(m_mediaArtDirectoryPath).entryInfoList(QStringList() << "album-?*-?*.jpeg",
                                                                             QDir::Files);

    if (mediaArtList.isEmpty())
        return QString();

    return mediaArtList.at(qrand() % mediaArtList.size()).filePath();
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

QString Utils::tracksSparqlQuery(bool allArtists,
                                 bool allAlbums,
                                 const QString &artist,
                                 bool unknownArtist,
                                 const QString &album,
                                 bool unknownAlbum)
{
    QString query =
            "SELECT ?title ?url ?duration ?artist ?rawArtist ?album ?rawAlbum\n"
            "WHERE {\n"
            "    {\n"
            "        SELECT tracker:coalesce(nie:title(?track), nfo:fileName(?track)) AS ?title\n"
            "               nie:url(?track) AS ?url\n"
            "               nfo:duration(?track) AS ?duration\n"
            "               nmm:trackNumber(?track) AS ?trackNumber\n"
            "               tracker:coalesce(nmm:artistName(nmm:performer(?track)), \"" + tr("Unknown artist") + "\") AS ?artist\n"
            "               nmm:artistName(nmm:performer(?track)) AS ?rawArtist\n"
            "               tracker:coalesce(nie:title(nmm:musicAlbum(?track)), \"" + tr("Unknown album") + "\") AS ?album\n"
            "               nie:title(nmm:musicAlbum(?track)) AS ?rawAlbum\n"
            "               nie:informationElementDate(?track) AS ?year\n"
            "        WHERE {\n"
            "            ?track a nmm:MusicPiece.\n"
            "        }\n"
            "        ORDER BY !bound(?rawArtist) ?rawArtist !bound(?rawAlbum) ?year ?rawAlbum ?trackNumber ?title\n"
            "    }.\n";

    if (!allArtists) {
        if (unknownArtist)
            query += "    FILTER(!bound(?rawArtist)).\n";
        else
            query += "    FILTER(?rawArtist = \"" + artist + "\").\n";
    }

    if (!allAlbums) {
        if (unknownAlbum)
            query += "    FILTER(!bound(?rawAlbum)).\n";
        else
            query += "    FILTER(?rawAlbum = \"" + album + "\").\n";
    }

    query += "}";

    return query;
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
