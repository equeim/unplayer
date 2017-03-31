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

#include <QDateTime>
#include <QFile>
#include <QImageReader>
#include <QLocale>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>

namespace unplayer
{
    Utils::Utils()
    {

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

    QString Utils::homeDirectory()
    {
        return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }

    QString Utils::sdcardPath(bool emptyIfNotMounted)
    {
        QFile mtab(QLatin1String("/etc/mtab"));
        if (mtab.open(QIODevice::ReadOnly)) {
            const QStringList mounts(QString::fromLatin1(mtab.readAll()).split('\n').filter(QLatin1String("/dev/mmcblk1p1")));
            if (!mounts.isEmpty()) {
                return mounts.first().split(' ').at(1);
            }
        }
        if (emptyIfNotMounted) {
            return QString();
        }
        return QLatin1String("/media/sdcard");
    }

    QStringList Utils::imageNameFilters()
    {
        QStringList nameFilters;
        for (const QByteArray& format : QImageReader::supportedImageFormats()) {
            nameFilters.append(QString::fromLatin1("*.%1").arg(QString::fromLatin1(format)));
        }
        return nameFilters;
    }

    QString Utils::urlToPath(const QUrl& url)
    {
        return url.path();
    }

    QString Utils::encodeUrl(const QUrl& url)
    {
        return QUrl::toPercentEncoding(url.toString(), "/!$&'()*+,;=:@");
    }
}
