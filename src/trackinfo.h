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

#include "tagutils.h"

namespace unplayer
{
    class TrackInfo : public QObject
    {
        Q_OBJECT

        Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY loaded)

        Q_PROPERTY(bool canReadTags READ canReadTags NOTIFY loaded)

        Q_PROPERTY(QString title READ title NOTIFY loaded)
        Q_PROPERTY(QStringList artists READ artists NOTIFY loaded)
        Q_PROPERTY(QString artist READ artist NOTIFY loaded)
        Q_PROPERTY(QStringList albumArtists READ albumArtists NOTIFY loaded)
        Q_PROPERTY(QString albumArtist READ albumArtist NOTIFY loaded)
        Q_PROPERTY(QStringList albums READ albums NOTIFY loaded)
        Q_PROPERTY(QString album READ album NOTIFY loaded)
        Q_PROPERTY(QString discNumber READ discNumber NOTIFY loaded)
        Q_PROPERTY(int year READ year NOTIFY loaded)
        Q_PROPERTY(int trackNumber READ trackNumber NOTIFY loaded)
        Q_PROPERTY(QStringList genres READ genres NOTIFY loaded)
        Q_PROPERTY(QString genre READ genre NOTIFY loaded)
        Q_PROPERTY(long long fileSize READ fileSize NOTIFY loaded)
        Q_PROPERTY(QString mimeType READ mimeType NOTIFY loaded)
        Q_PROPERTY(int duration READ duration NOTIFY loaded)
        Q_PROPERTY(QString bitrate READ bitrate NOTIFY loaded)

    public:
        const QString& filePath() const;
        void setFilePath(const QString& filePath);

        bool canReadTags() const;

        const QString& title() const;
        const QStringList& artists() const;
        QString artist() const;
        const QStringList& albumArtists() const;
        QString albumArtist() const;
        const QStringList& albums() const;
        QString album() const;
        const QString& discNumber() const;
        int year() const;
        int trackNumber() const;
        const QStringList& genres() const;
        QString genre() const;
        long long fileSize() const;
        const QString& mimeType() const;
        int duration() const;
        QString bitrate() const;

    private:
        QString mFilePath;
        tagutils::Info mInfo;
        QString mMimeType;
        long long mFileSize;

    signals:
        void loaded();
    };
}

#endif // UNPLAYER_TRACKINFO_H
