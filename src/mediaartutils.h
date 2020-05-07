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

#ifndef UNPLAYER_MEDIAARTPROVIDER_H
#define UNPLAYER_MEDIAARTPROVIDER_H

#include <atomic>
#include <unordered_map>

#include <QObject>

class QMimeDatabase;
class QThread;

namespace unplayer
{
    class MediaArtUtilsWorker;

    class MediaArtUtils : public QObject
    {
        Q_OBJECT
    public:
        static MediaArtUtils* instance();
        static void deleteInstance();

        static const QString& mediaArtDirectory();
        static QString findMediaArtForDirectory(std::unordered_map<QString, QString>& mediaArtHash, const QString& directoryPath, const std::atomic_bool& cancelFlag = false);
        static std::unordered_map<QByteArray, QString> getEmbeddedMediaArtFiles();
        static QString saveEmbeddedMediaArt(const QByteArray& data, std::unordered_map<QByteArray, QString>& embeddedMediaArtFiles, const QMimeDatabase& mimeDb);

        Q_INVOKABLE void setUserMediaArt(int albumId, const QString& mediaArt);

        void getMediaArtForFile(const QString& filePath, const QString& albumForUserMediaArt, bool onlyExtractEmbedded);

        void getRandomMediaArt(uintptr_t requestId);
        void getRandomMediaArtForArtist(int artistId);
        void getRandomMediaArtForAlbum(int albumId);
        void getRandomMediaArtForArtistAndAlbum(int artistId, int albumId);
        void getRandomMediaArtForGenre(int genreId);

    private:
        explicit MediaArtUtils(QObject* parent);
        ~MediaArtUtils();

        void createWorker();

        QThread* mWorkerThread = nullptr;
        MediaArtUtilsWorker* mWorker = nullptr;

    signals:
        void gotMediaArtForFile(const QString& filePath, const QString& mediaArt, const QByteArray& embeddedMediaArtData);

        void gotRandomMediaArt(uintptr_t requestId, const QString& mediaArt);
        void gotRandomMediaArtForArtist(int artistId, const QString& mediaArt);
        void gotRandomMediaArtForAlbum(int albumId, const QString& mediaArt);
        void gotRandomMediaArtForArtistAndAlbum(int artistId, int albumId, const QString& mediaArt);
        void gotRandomMediaArtForGenre(int genreId, const QString& mediaArt);

        void mediaArtChanged();
    };

    class RandomMediaArt : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString mediaArt READ mediaArt NOTIFY mediaArtChanged)
    public:
        RandomMediaArt();
        const QString& mediaArt();

    private:
        QString mMediaArt;
        bool mMediaArtRequested = false;

    signals:
        void mediaArtChanged(const QString& mediaArt);
    };
}

#endif // UNPLAYER_MEDIAARTPROVIDER_H
