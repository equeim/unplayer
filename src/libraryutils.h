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

#ifndef UNPLAYER_LIBRARYUTILS_H
#define UNPLAYER_LIBRARYUTILS_H

#include <QObject>

class QFileInfo;
class QSqlDatabase;

namespace unplayer
{
    struct LibraryTrackInfo
    {
        long long modificationTime;
        QString title;
        QString artist;
        QString album;
        int year;
        int trackNumber;
        QString genre;
        int duration;
    };

    class LibraryUtils : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(bool databaseInitialized READ isDatabaseInitialized CONSTANT)
        Q_PROPERTY(bool createdTable READ isCreatedTable CONSTANT)
        Q_PROPERTY(bool updating READ isUpdating NOTIFY updatingChanged)
        Q_PROPERTY(int artistsCount READ artistsCount NOTIFY databaseChanged)
        Q_PROPERTY(int albumsCount READ albumsCount NOTIFY databaseChanged)
        Q_PROPERTY(int tracksCount READ tracksCount NOTIFY databaseChanged)
        Q_PROPERTY(int tracksDuration READ tracksDuration NOTIFY databaseChanged)
        Q_PROPERTY(QString randomMediaArt READ randomMediaArt NOTIFY mediaArtChanged)
    public:
        static const QVector<QString> supportedMimeTypes;
        static LibraryUtils* instance();

        static QString databaseFilePath();

        static LibraryTrackInfo getTrackInfo(const QFileInfo& fileInfo);
        static QString findMediaArtForDirectory(QHash<QString, QString>& directoriesHash, const QString& directoryPath);

        static void initDatabase();
        Q_INVOKABLE void updateDatabase();
        Q_INVOKABLE void resetDatabase();

        static bool isDatabaseInitialized();
        static bool isCreatedTable();
        static bool isUpdating();

        static int artistsCount();
        static int albumsCount();
        static int tracksCount();
        static int tracksDuration();
        static QString randomMediaArt();
        Q_INVOKABLE static QString randomMediaArtForArtist(const QString& artist);
        Q_INVOKABLE static QString randomMediaArtForAlbum(const QString& artist, const QString& album);

        Q_INVOKABLE void setMediaArt(const QString& artist, const QString& album, const QString& mediaArt);
    private:
        explicit LibraryUtils(QObject* parent);
    signals:
        void updatingChanged();
        void databaseChanged();
        void mediaArtChanged();
    };
}

#endif // UNPLAYER_LIBRARYUTILS_H
