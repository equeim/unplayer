 /*
 * Unplayer
 * Copyright (C) 2015-2020 Alexey Rochev <equeim@gmail.com>
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

#ifndef UNPLAYER_LIBRARYTRACKSADDER_H
#define UNPLAYER_LIBRARYTRACKSADDER_H

#include <unordered_map>

#include <QLatin1String>
#include <QPair>
#include <QSqlQuery>
#include <QString>
#include <QVector>

#include "stdutils.h"

class QSqlDatabase;

using QStringIntVectorPair = QPair<QString, QVector<int>>;
QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF(QStringIntVectorPair)

namespace unplayer
{
    namespace tagutils
    {
        struct Info;
    }

    class LibraryTracksAdder
    {
    public:
        explicit LibraryTracksAdder(const QSqlDatabase& db);

        void addTrackToDatabase(const QString& filePath,
                                long long modificationTime,
                                tagutils::Info& info,
                                const QString& directoryMediaArt,
                                const QString& embeddedMediaArt);

        int getAddedArtistId(const QString& title);
        int getAddedAlbumId(const QString& title, const QVector<int>& artistIds);

    private:
        struct ArtistsOrGenres
        {
            explicit ArtistsOrGenres(QLatin1String table, const QSqlDatabase& db);

            QLatin1String table;
            QSqlQuery addNewQuery;
            QSqlQuery addTracksRelationshipQuery;

            std::unordered_map<QString, int> ids{};
            int lastId = 0;
        };

        struct Albums
        {
            explicit Albums(const QSqlDatabase& db);

            QSqlQuery addNewQuery;
            QSqlQuery addTracksRelationshipQuery;
            QSqlQuery addArtistsRelationshipQuery;

            std::unordered_map<QPair<QString, QVector<int>>, int> ids{};
            int lastId = 0;
        };

        void getArtists();
        void getGenres();
        void getArtistsOrGenres(ArtistsOrGenres& ids);
        void getAlbums();

        int getArtistId(const QString& title);
        int getGenreId(const QString& title);
        int getArtistOrGenreId(const QString& title, ArtistsOrGenres& ids);
        int addArtistOrGenre(const QString& title, ArtistsOrGenres& ids);

        int getAlbumId(const QString& title, QVector<int>&& artistIds);
        int addAlbum(const QString& title, QVector<int>&& artistIds);

        void addRelationship(int firstId, int secondId, QSqlQuery& query);

        int mLastTrackId;
        const QSqlDatabase& mDb;
        QSqlQuery mAddTrackQuery{mDb};

        ArtistsOrGenres mArtists{QLatin1String("artists"), mDb};
        Albums mAlbums{mDb};
        ArtistsOrGenres mGenres{QLatin1String("genres"), mDb};
    };
}

#endif // UNPLAYER_LIBRARYTRACKSADDER_H
