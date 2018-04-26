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

#ifndef UNPLAYER_TRACKINFO_H
#define UNPLAYER_TRACKINFO_H

#include <QObject>

namespace unplayer
{
    class TrackInfo : public QObject
    {
        Q_OBJECT

        Q_PROPERTY(QString filePath READ filePath WRITE setFilePath)

        Q_PROPERTY(QString title READ title CONSTANT)
        Q_PROPERTY(QString artist READ artist CONSTANT)
        Q_PROPERTY(QString album READ album CONSTANT)
        Q_PROPERTY(int year READ year CONSTANT)
        Q_PROPERTY(int trackNumber READ trackNumber CONSTANT)
        Q_PROPERTY(QString genre READ genre CONSTANT)
        Q_PROPERTY(int fileSize READ fileSize CONSTANT)
        Q_PROPERTY(QString mimeType READ mimeType CONSTANT)
        Q_PROPERTY(int duration READ duration CONSTANT)
        Q_PROPERTY(QString bitrate READ bitrate CONSTANT)
    public:
        const QString& filePath() const;
        void setFilePath(const QString& filePath);

        const QString& title() const;
        const QString& artist() const;
        const QString& album() const;
        int year() const;
        int trackNumber() const;
        const QString& genre() const;
        int fileSize() const;
        const QString& mimeType() const;
        int duration() const;
        QString bitrate() const;

    private:
        QString mFilePath;

        QString mTitle;
        QString mArtist;
        QString mAlbum;
        int mYear = 0;
        int mTrackNumber = 0;
        QString mGenre;
        int mFileSize = 0;
        QString mMimeType;
        int mDuration = 0;
        int mBitrate = 0;
    };
}

#endif // UNPLAYER_TRACKINFO_H
