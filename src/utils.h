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

#ifndef UNPLAYER_UTILS_H
#define UNPLAYER_UTILS_H

#include <QObject>
#include <QUrl>

namespace Unplayer
{

class Utils : public QObject
{
    Q_OBJECT
public:
    Utils();

    Q_INVOKABLE static QUrl mediaArt(const QString &artist, const QString &album, const QString &trackUrl = QString());
    Q_INVOKABLE static QUrl mediaArtForArtist(const QString &artist);
    Q_INVOKABLE static QUrl randomMediaArt();
    Q_INVOKABLE static void setMediaArt(const QString &filePath, const QString &artist, const QString &album);

    Q_INVOKABLE static QString formatDuration(uint seconds);
    Q_INVOKABLE static QString formatByteSize(double size);

    Q_INVOKABLE static QString escapeRegExp(const QString &string);
    Q_INVOKABLE static QString escapeSparql(QString string);

    Q_INVOKABLE static QString tracksSparqlQuery(bool allArtists,
                                                 bool allAlbums,
                                                 const QString &artist,
                                                 bool unknownArtist,
                                                 const QString &album,
                                                 bool unknownAlbum);

    Q_INVOKABLE static QString homeDirectory();
    Q_INVOKABLE static QString sdcard();
    Q_INVOKABLE static QString urlToPath(const QUrl &url);
private:
    static QString mediaArtPath(const QString &artist, const QString &album);
    static QString mediaArtMd5(QString string);
private:
    static const QString m_mediaArtDirectory;
    static const QString m_homeDirectory;
};

}

#endif // UNPLAYER_UTILS_H
