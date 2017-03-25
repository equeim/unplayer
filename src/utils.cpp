/*
 * Unplayer
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QLocale>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>

namespace unplayer
{
    namespace
    {
        const QString mediaArtDirectory(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + "/media-art");
    }

    Utils::Utils()
    {
        qsrand(QDateTime::currentMSecsSinceEpoch());
    }

    QUrl Utils::mediaArt(const QString& artist, const QString& album, const QString& trackUrl)
    {
        if (!artist.isEmpty() && !album.isEmpty()) {
            const QString filePath(mediaArtPath(artist, album));
            if (QFileInfo(filePath).exists()) {
                return QUrl::fromLocalFile(filePath);
            }
        }

        if (trackUrl.isEmpty()) {
            return QUrl();
        }

        const QString trackDirectory(QFileInfo(QUrl(trackUrl).path()).path());

        const QStringList foundMediaArt(
            QDir(trackDirectory)
                .entryList(QDir::Files)
                .filter(QRegularExpression(QStringLiteral("^(albumart.*|cover|folder|front)\\.(jpeg|jpg|png)$"),
                                           QRegularExpression::CaseInsensitiveOption)));

        if (foundMediaArt.isEmpty()) {
            return QUrl();
        }

        return QUrl::fromLocalFile(QStringLiteral("%1/%2").arg(trackDirectory).arg(foundMediaArt.first()));
    }

    QUrl Utils::mediaArtForArtist(const QString& artist)
    {
        if (artist.isEmpty()) {
            return QString();
        }

        const QFileInfoList mediaArtList(QDir(mediaArtDirectory)
                                             .entryInfoList((QStringList{QStringLiteral("album-%1-?*.jpeg").arg(mediaArtMd5(artist))}), QDir::Files));

        if (mediaArtList.isEmpty()) {
            return QString();
        }

        static const int random = qrand();
        return QUrl::fromLocalFile(mediaArtList.at(random % mediaArtList.size()).filePath());
    }

    QUrl Utils::randomMediaArt()
    {
        const QFileInfoList mediaArtList(QDir(mediaArtDirectory).entryInfoList((QStringList{QStringLiteral("album-?*-?*.jpeg")}), QDir::Files));

        if (mediaArtList.isEmpty()) {
            return QString();
        }

        return QUrl::fromLocalFile(mediaArtList.at(qrand() % mediaArtList.size()).filePath());
    }

    void Utils::setMediaArt(const QString& filePath, const QString& artist, const QString& album)
    {
        const QString newFilePath(mediaArtPath(artist, album));
        QFile::remove(newFilePath);

        if (filePath.endsWith(".jpeg") || filePath.endsWith(".jpg")) {
            QFile::copy(filePath, newFilePath);
        } else {
            QImage(filePath).save(newFilePath);
        }
    }

    QString Utils::formatDuration(uint seconds)
    {
        const int hours = seconds / 3600;
        seconds %= 3600;
        const int minutes = seconds / 60;
        seconds %= 60;

        const QLocale locale;

        if (hours > 0) {
            return tr("%1 h %2 m").arg(locale.toString(hours)).arg(locale.toString(minutes));
        }

        if (minutes > 0) {
            return tr("%1 m %2 s").arg(locale.toString(minutes)).arg(locale.toString(seconds));
        }

        return tr("%1 s").arg(locale.toString(seconds));
    }

    QString Utils::formatByteSize(double size)
    {
        int unit = 0;
        while (size >= 1024.0 && unit < 8) {
            size /= 1024.0;
            unit++;
        }

        QString string;
        if (unit == 0) {
            string = QString::number(size);
        } else {
            string = QLocale().toString(size, 'f', 1);
        }

        switch (unit) {
        case 0:
            return tr("%1 B").arg(string);
        case 1:
            return tr("%1 KiB").arg(string);
        case 2:
            return tr("%1 MiB").arg(string);
        case 3:
            return tr("%1 GiB").arg(string);
        case 4:
            return tr("%1 TiB").arg(string);
        case 5:
            return tr("%1 PiB").arg(string);
        case 6:
            return tr("%1 EiB").arg(string);
        case 7:
            return tr("%1 ZiB").arg(string);
        case 8:
            return tr("%1 YiB").arg(string);
        }

        return QString();
    }

    QString Utils::escapeRegExp(const QString& string)
    {
        return QRegularExpression::escape(string);
    }

    QString Utils::escapeSparql(QString string)
    {
        return string.replace("\\", "\\\\").replace("\t", "\\t").replace("\n", "\\n").replace("\r", "\\r").replace("\b", "\\b").replace("\f", "\\f").replace("\"", "\\\"").replace("'", "\\'");
    }

    QString Utils::tracksSparqlQuery(bool allArtists,
                                     bool allAlbums,
                                     const QString& artist,
                                     const QString& album,
                                     const QString& genre)
    {
        QString query(QStringLiteral(
                          "SELECT ?url ?title ?artist ?rawArtist ?album ?rawAlbum ?trackNumber ?genre ?duration\n"
                          "WHERE {\n"
                          "    {\n"
                          "        SELECT nie:url(?track) AS ?url\n"
                          "               tracker:coalesce(nie:title(?track), nfo:fileName(?track)) AS ?title\n"
                          "               tracker:coalesce(nmm:artistName(nmm:performer(?track)), \"%1\") AS ?artist\n"
                          "               nmm:artistName(nmm:performer(?track)) AS ?rawArtist\n"
                          "               tracker:coalesce(nie:title(nmm:musicAlbum(?track)), \"%2\") AS ?album\n"
                          "               nie:title(nmm:musicAlbum(?track)) AS ?rawAlbum\n"
                          "               nie:informationElementDate(?track) AS ?date\n"
                          "               nmm:trackNumber(?track) AS ?trackNumber\n"
                          "               nfo:genre(?track) AS ?genre\n"
                          "               nfo:duration(?track) AS ?duration\n"
                          "        WHERE {\n"
                          "            ?track a nmm:MusicPiece.\n"
                          "        }\n"
                          "        ORDER BY !bound(?rawArtist) ?artist !bound(?rawAlbum) ?date ?album ?trackNumber ?title\n"
                          "    }.\n")
                          .arg(tr("Unknown artist"))
                          .arg(tr("Unknown album")));

        if (!allArtists) {
            if (artist.isEmpty()) {
                query += "    FILTER(!bound(?rawArtist)).\n";
            } else {
                query += QStringLiteral("    FILTER(?rawArtist = \"%1\").\n").arg(escapeSparql(artist));
            }
        }

        if (!allAlbums) {
            if (album.isEmpty()) {
                query += "    FILTER(!bound(?rawAlbum)).\n";
            } else {
                query += QStringLiteral("    FILTER(?rawAlbum = \"%1\").\n").arg(escapeSparql(album));
            }
        }

        if (!genre.isEmpty()) {
            query += QStringLiteral("    FILTER(?genre = \"%1\").\n").arg(escapeSparql(genre));
        }

        query += "}";

        return query;
    }

    QString Utils::homeDirectory()
    {
        return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }

    QString Utils::sdcard()
    {
        QFile mtab(QStringLiteral("/etc/mtab"));
        if (mtab.open(QIODevice::ReadOnly)) {
            const QStringList mounts(QString(mtab.readAll()).split('\n').filter(QStringLiteral("/dev/mmcblk1p1")));
            if (!mounts.isEmpty()) {
                return mounts.first().split(' ').at(1);
            }
        }
        return QStringLiteral("/media/sdcard");
    }

    QStringList Utils::imageNameFilters()
    {
        QStringList nameFilters;
        for (const QByteArray& format : QImageReader::supportedImageFormats()) {
            nameFilters.append("*." + format);
        }
        return nameFilters;
    }

    QString Utils::urlToPath(const QUrl& url)
    {
        return url.path();
    }

    QString Utils::encodeUrl(const QUrl& url)
    {
        return QUrl::toPercentEncoding(url.toString(), QByteArrayLiteral("/!$&'()*+,;=:@"));
    }

    QString Utils::mediaArtPath(const QString& artist, const QString& album)
    {
        return QStringLiteral("%1/album-%2-%3.jpeg").arg(mediaArtDirectory).arg(mediaArtMd5(artist)).arg(mediaArtMd5(album));
    }

    QString Utils::mediaArtMd5(QString string)
    {
        string = string.replace(QRegularExpression(QStringLiteral("\\([^\\)]*\\)")), QString())
                     .replace(QRegularExpression(QStringLiteral("\\{[^\\}]*\\}")), QString())
                     .replace(QRegularExpression(QStringLiteral("\\[[^\\]]*\\]")), QString())
                     .replace(QRegularExpression(QStringLiteral("<[^>]*>")), QString())
                     .replace(QRegularExpression(QStringLiteral("[\\(\\)_\\{\\}\\[\\]!@#\\$\\^&\\*\\+=|/\\\'\"?<>~`]")), QString())
                     .trimmed()
                     .replace('\t', ' ')
                     .replace(QRegularExpression(QStringLiteral("  +")), QStringLiteral(" "))
                     .normalized(QString::NormalizationForm_KD)
                     .toLower();

        if (string.isEmpty()) {
            string = " ";
        }

        return QCryptographicHash::hash(string.toUtf8(), QCryptographicHash::Md5).toHex();
    }
}
