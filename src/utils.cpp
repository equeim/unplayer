/*
 * Unplayer
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
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

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QItemSelection>
#include <QLocale>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>
#include <qqml.h>

#include "albumsmodel.h"
#include "artistsmodel.h"
#include "directorycontentmodel.h"
#include "directorycontentproxymodel.h"
#include "directorytracksmodel.h"
#include "filterproxymodel.h"
#include "genresmodel.h"
#include "librarydirectoriesmodel.h"
#include "libraryutils.h"
#include "player.h"
#include "playlistmodel.h"
#include "playlistsmodel.h"
#include "playlistutils.h"
#include "queue.h"
#include "queuemodel.h"
#include "settings.h"
#include "stdutils.h"
#include "trackinfo.h"
#include "tracksmodel.h"

Q_DECLARE_METATYPE(unplayer::LibraryTrack)
Q_DECLARE_METATYPE(std::vector<unplayer::LibraryTrack>)

namespace unplayer
{
    void Utils::registerTypes()
    {
        qRegisterMetaType<std::vector<int>>();
        qRegisterMetaType<LibraryTrack>();
        qRegisterMetaType<std::vector<LibraryTrack>>();

        const char* url = "harbour.unplayer";
        const int major = 0;
        const int minor = 1;

        qmlRegisterSingletonType<Settings>(url, major, minor, "Settings", [](QQmlEngine*, QJSEngine*) -> QObject* { return Settings::instance(); });
        qmlRegisterSingletonType<LibraryUtils>(url, major, minor, "LibraryUtils", [](QQmlEngine*, QJSEngine*) -> QObject* { return LibraryUtils::instance(); });

        qmlRegisterSingletonType<Player>(url, major, minor, "Player", [](QQmlEngine*, QJSEngine*) -> QObject* { return Player::instance(); });

        qmlRegisterUncreatableType<Queue>(url, major, minor, "Queue", QString());
        qmlRegisterType<QueueModel>(url, major, minor, "QueueModel");

        qmlRegisterType<ArtistsModel>(url, major, minor, "ArtistsModel");
        qmlRegisterType<AlbumsModel>(url, major, minor, "AlbumsModel");

        qmlRegisterType<TracksModel>(url, major, minor, "TracksModel");
        qmlRegisterUncreatableType<TracksModelSortMode>(url, major, minor, "TracksModelSortMode", QString());
        qmlRegisterUncreatableType<TracksModelInsideAlbumSortMode>(url, major, minor, "TracksModelInsideAlbumSortMode", QString());

        qmlRegisterType<GenresModel>(url, major, minor, "GenresModel");

        qmlRegisterSingletonType<PlaylistUtils>(url, major, minor, "PlaylistUtils", [](QQmlEngine*, QJSEngine*) -> QObject* { return PlaylistUtils::instance(); });
        qmlRegisterType<PlaylistsModel>(url, major, minor, "PlaylistsModel");
        qmlRegisterType<PlaylistModel>(url, major, minor, "PlaylistModel");

        qmlRegisterType<DirectoryTracksModel>(url, major, minor, "DirectoryTracksModel");
        qmlRegisterType<DirectoryTracksProxyModel>(url, major, minor, "DirectoryTracksProxyModel");

        qmlRegisterType<DirectoryContentModel>(url, major, minor, "DirectoryContentModel");
        qmlRegisterType<DirectoryContentProxyModel>(url, major, minor, "DirectoryContentProxyModel");

        qmlRegisterType<FilterProxyModel>(url, major, minor, "FilterProxyModel");
        qmlRegisterType<QItemSelectionModel>();
        qmlRegisterType<QAbstractItemModel>();

        qmlRegisterSingletonType<Utils>(url, major, minor, "Utils", [](QQmlEngine*, QJSEngine*) -> QObject* { return new Utils(); });
        qmlRegisterType<TrackInfo>(url, major, minor, "TrackInfo");
        qmlRegisterType<LibraryDirectoriesModel>(url, major, minor, "LibraryDirectoriesModel");
    }

    QStringList Utils::processArguments(const std::vector<std::string>& arguments)
    {
        QStringList parsed;
        parsed.reserve(arguments.size());

        const QDir currentDir;

        for (const std::string& argument : arguments) {
            const QString argumentString(QString::fromStdString(argument));
            const QUrl url(argumentString);
            if (url.isRelative() || url.isLocalFile()) {
                QFileInfo fileInfo(url.path());
                if (fileInfo.isFile() && fileInfo.isReadable()) {
                    QString filePath(fileInfo.absoluteFilePath());
                    if (contains(PlaylistUtils::playlistsExtensions, fileInfo.suffix())) {
                        parsed.push_back(filePath);
                    } else {
                        const QString suffix(fileInfo.suffix());
                        if (contains(LibraryUtils::mimeTypesExtensions, suffix)
                                || contains(LibraryUtils::videoMimeTypesExtensions, suffix)) {
                            parsed.push_back(filePath);
                        }
                    }
                }
            } else {
                parsed.push_back(argumentString);
            }
        }
        return parsed;
    }

    QString Utils::formatDuration(uint seconds)
    {
        const int hours = seconds / 3600;
        seconds %= 3600;
        const int minutes = seconds / 60;
        seconds %= 60;

        const QLocale locale;

        if (hours > 0) {
            return qApp->translate("unplayer", "%1 h %2 m").arg(locale.toString(hours), locale.toString(minutes));
        }

        if (minutes > 0) {
            return qApp->translate("unplayer", "%1 m %2 s").arg(locale.toString(minutes), locale.toString(seconds));
        }

        return qApp->translate("unplayer", "%1 s").arg(locale.toString(seconds));
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
            return qApp->translate("unplayer", "%1 B").arg(string);
        case 1:
            return qApp->translate("unplayer", "%1 KiB").arg(string);
        case 2:
            return qApp->translate("unplayer", "%1 MiB").arg(string);
        case 3:
            return qApp->translate("unplayer", "%1 GiB").arg(string);
        case 4:
            return qApp->translate("unplayer", "%1 TiB").arg(string);
        case 5:
            return qApp->translate("unplayer", "%1 PiB").arg(string);
        case 6:
            return qApp->translate("unplayer", "%1 EiB").arg(string);
        case 7:
            return qApp->translate("unplayer", "%1 ZiB").arg(string);
        case 8:
            return qApp->translate("unplayer", "%1 YiB").arg(string);
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
            const QStringList mounts(QString::fromLatin1(mtab.readAll()).split(QLatin1Char('\n')).filter(QLatin1String("/dev/mmcblk1p1")));
            if (!mounts.isEmpty()) {
                return mounts.first().split(QLatin1Char(' ')).at(1);
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
        const QList<QByteArray> formats(QImageReader::supportedImageFormats());
        nameFilters.reserve(formats.size());
        for (const QByteArray& format : formats) {
            nameFilters.append(QString::fromLatin1("*.%1").arg(QString::fromLatin1(format)));
        }
        return nameFilters;
    }

    QString Utils::translators()
    {
        QFile file(QLatin1String(":/translators.html"));
        file.open(QIODevice::ReadOnly);
        return QString(file.readAll());
    }

    QString Utils::license()
    {
        QFile file(QLatin1String(":/license.html"));
        file.open(QIODevice::ReadOnly);
        return QLatin1String(file.readAll());
    }
}
