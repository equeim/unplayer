/*
 * Unplayer
 * Copyright (C) 2015-2019 Alexey Rochev <equeim@gmail.com>
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

#include "trackinfo.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QMimeDatabase>

namespace unplayer
{
    namespace
    {
        inline QString joinList(const QStringList& list)
        {
            return list.join(QLatin1String(", "));
        }
    }

    const QString& TrackInfo::filePath() const
    {
        return mFilePath;
    }

    void TrackInfo::setFilePath(const QString& filePath)
    {
        if (filePath != mFilePath) {
            mFilePath = filePath;
            const QFileInfo fileInfo(mFilePath);
            mFileName = fileInfo.fileName();
            const QMimeDatabase mimeDb;
            mInfo = tagutils::getTrackInfo(mFilePath, fileutils::extensionFromSuffix(fileInfo.suffix()), mimeDb);
            mFileSize = fileInfo.size();
            mMimeType = QMimeDatabase().mimeTypeForFile(mFilePath, QMimeDatabase::MatchContent).name();
            emit loaded();
        }
    }

    const QString& TrackInfo::fileName() const
    {
        return mFileName;
    }

    bool TrackInfo::canReadTags() const
    {
        return mInfo.canReadTags;
    }

    const QString& TrackInfo::title() const
    {
        return mInfo.title;
    }

    const QStringList& TrackInfo::artists() const
    {
        return mInfo.artists;
    }

    QString TrackInfo::artist() const
    {
        return joinList(mInfo.artists);
    }

    const QStringList& TrackInfo::albumArtists() const
    {
        return mInfo.albumArtists;
    }

    QString TrackInfo::albumArtist() const
    {
        return joinList(mInfo.albumArtists);
    }

    const QStringList& TrackInfo::albums() const
    {
        return mInfo.albums;
    }

    QString TrackInfo::album() const
    {
        return joinList(mInfo.albums);
    }

    const QString& TrackInfo::discNumber() const
    {
        return mInfo.discNumber;
    }

    int TrackInfo::year() const
    {
        return mInfo.year;
    }

    int TrackInfo::trackNumber() const
    {
        return mInfo.trackNumber;
    }

    const QStringList& TrackInfo::genres() const
    {
        return mInfo.genres;
    }

    QString TrackInfo::genre() const
    {
        return joinList(mInfo.genres);
    }

    long long TrackInfo::fileSize() const
    {
        return mFileSize;
    }

    const QString& TrackInfo::mimeType() const
    {
        return mMimeType;
    }

    int TrackInfo::duration() const
    {
        return mInfo.duration;
    }

    QString TrackInfo::bitrate() const
    {
        return qApp->translate("unplayer", "%1 kB/s").arg(mInfo.bitrate);
    }
}
