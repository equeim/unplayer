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

#include "trackinfo.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QMimeDatabase>

#include <fileref.h>
#include <tag.h>

namespace unplayer
{
    const QString& TrackInfo::filePath() const
    {
        return mFilePath;
    }

    void TrackInfo::setFilePath(const QString& filePath)
    {
        mFilePath = filePath;

        TagLib::FileRef file(mFilePath.toUtf8().constData());

        const TagLib::Tag* tag = file.tag();
        if (tag) {
            mTitle = tag->title().toCString(true);
            mArtist = tag->artist().toCString(true);
            mAlbum = tag->album().toCString(true);
            mTrackNumber = tag->track();
            mGenre = tag->genre().toCString(true);
        }

        mFileSize = QFileInfo(mFilePath).size();

        mMimeType = QMimeDatabase().mimeTypeForFile(mFilePath, QMimeDatabase::MatchExtension).name();

        const TagLib::AudioProperties* audioProperties = file.audioProperties();
        if (audioProperties) {
            mDuration = audioProperties->length();
            mBitrate = audioProperties->bitrate();
            mHasAudioProperties = true;
        }
    }

    const QString& TrackInfo::title() const
    {
        return mTitle;
    }

    const QString& TrackInfo::artist() const
    {
        return mArtist;
    }

    const QString& TrackInfo::album() const
    {
        return mAlbum;
    }

    int TrackInfo::trackNumber() const
    {
        return mTrackNumber;
    }

    const QString& TrackInfo::genre() const
    {
        return mGenre;
    }

    int TrackInfo::fileSize() const
    {
        return mFileSize;
    }

    const QString& TrackInfo::mimeType() const
    {
        return mMimeType;
    }

    int TrackInfo::duration() const
    {
        return mDuration;
    }

    QString TrackInfo::bitrate() const
    {
        return qApp->translate("unplayer", "%1 kB/s").arg(mBitrate);
    }

    bool TrackInfo::hasAudioProperties() const
    {
        return mHasAudioProperties;
    }
}
