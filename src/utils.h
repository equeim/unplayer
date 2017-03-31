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

#ifndef UNPLAYER_UTILS_H
#define UNPLAYER_UTILS_H

#include <QObject>
#include <QStringList>
#include <QUrl>

namespace unplayer
{
    class Utils : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString homeDirectory READ homeDirectory)
        Q_PROPERTY(QString sdcardPath READ sdcardPath)
        Q_PROPERTY(QStringList imageNameFilters READ imageNameFilters CONSTANT)
    public:
        Utils();

        Q_INVOKABLE static QString formatDuration(uint seconds);
        Q_INVOKABLE static QString formatByteSize(double size);

        Q_INVOKABLE static QString escapeRegExp(const QString& string);

        static QString homeDirectory();
        static QString sdcardPath(bool emptyIfNotMounted = false);

        static QStringList imageNameFilters();

        Q_INVOKABLE static QString urlToPath(const QUrl& url);
        static QString encodeUrl(const QUrl& url);
    };
}

#endif // UNPLAYER_UTILS_H
