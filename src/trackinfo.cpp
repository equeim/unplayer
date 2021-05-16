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
#include <QtConcurrentRun>

#include "fileutils.h"
#include "utilsfunctions.h"

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
            mInfo = tagutils::getTrackInfo(mFilePath, fileutils::extensionFromSuffix(fileInfo.suffix())).value_or(tagutils::Info{});
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

    QString TrackInfo::audioCodec() const
    {
        return fileutils::audioCodecDisplayName(mInfo.audioCodec);
    }

    int TrackInfo::duration() const
    {
        return mInfo.duration;
    }

    QString TrackInfo::bitrate() const
    {
        return qApp->translate("unplayer", "%1 kbit/s").arg(mInfo.bitrate);
    }

    bool TrackInfo::hasBitDepth() const
    {
        return mInfo.bitDepth != 0;
    }

    QString TrackInfo::bitDepth() const
    {
        return qApp->translate("unplayer", "%1 bits").arg(mInfo.bitDepth);
    }

    QString TrackInfo::sampleRate() const
    {
        return qApp->translate("unplayer", "%1 Hz").arg(mInfo.sampleRate);
    }

    int TrackInfo::channels() const
    {
        return mInfo.channels;
    }

    bool TrackAudioCodecInfo::isLoaded() const
    {
        return mLoaded;
    }

    int TrackAudioCodecInfo::sampleRate() const
    {
        return mInfo.sampleRate;
    }

    int TrackAudioCodecInfo::bitDepth() const
    {
        return mInfo.bitDepth;
    }

    int TrackAudioCodecInfo::bitrate() const
    {
        return mInfo.bitrate;
    }

    QString TrackAudioCodecInfo::audioCodec() const
    {
        return fileutils::audioCodecDisplayName(mInfo.audioCodec);
    }

    const QString& TrackAudioCodecInfo::filePath() const
    {
        return mFilePath;
    }

    void TrackAudioCodecInfo::setFilePath(const QString& filePath)
    {
        if (filePath != mFilePath) {
            mFilePath = filePath;
            emit filePathChanged(filePath);
            if (mLoaded) {
                mInfo = {};
                mLoaded = false;
                emit loadedChanged(mLoaded);
            }
            if (!filePath.isEmpty()) {
                const auto future(QtConcurrent::run([filePath] {
                    return tagutils::getTrackAudioCodecInfo(filePath, fileutils::extensionFromSuffix(QFileInfo(filePath).suffix()));
                }));
                onFutureFinished(future, this, [=](const std::optional<tagutils::AudioCodecInfo>& info) {
                    if (mFilePath == filePath && !mLoaded) {
                        mInfo = info.value_or(tagutils::AudioCodecInfo{});
                        mLoaded = true;
                        emit loadedChanged(true);
                    }
                });
            }
        }
    }
}
