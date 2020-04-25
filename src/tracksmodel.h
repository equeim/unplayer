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

#ifndef UNPLAYER_TRACKSMODEL_H
#define UNPLAYER_TRACKSMODEL_H

#include <vector>

#include <QAbstractListModel>
#include <QQmlParserStatus>

#include "abstractlibrarymodel.h"
#include "librarytrack.h"

namespace unplayer
{
    struct TracksModelSortMode
    {
        Q_GADGET
    public:
        enum Mode
        {
            Title,
            AddedDate,
            ArtistAlbumTitle,
            ArtistAlbumYear
        };
        Q_ENUM(Mode)
    };

    struct TracksModelInsideAlbumSortMode
    {
        Q_GADGET
    public:
        enum Mode
        {
            Title,
            DiscNumberTitle,
            DiscNumberTrackNumber
        };
        Q_ENUM(Mode)
    };

    class TracksModel : public AbstractLibraryModel<LibraryTrack>, public QQmlParserStatus
    {
        Q_OBJECT

        Q_INTERFACES(QQmlParserStatus)

        Q_PROPERTY(Mode mode READ mode WRITE setMode)
        Q_PROPERTY(int artistId READ artistId WRITE setArtistId)
        Q_PROPERTY(int albumId READ albumId WRITE setAlbumId)
        Q_PROPERTY(int genreId READ genreId WRITE setGenreId)

        Q_PROPERTY(bool sortDescending READ sortDescending WRITE setSortDescending)
        Q_PROPERTY(unplayer::TracksModelSortMode::Mode sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
        Q_PROPERTY(unplayer::TracksModelInsideAlbumSortMode::Mode insideAlbumSortMode READ insideAlbumSortMode WRITE setInsideAlbumSortMode NOTIFY insideAlbumSortModeChanged)
    public:
        enum Role
        {
            FilePathRole = Qt::UserRole,
            TitleRole,
            ArtistRole,
            AlbumRole,
            DurationRole
        };
        Q_ENUM(Role)

        enum Mode
        {
            AllTracksMode,
            ArtistMode,
            AlbumAllArtistsMode,
            AlbumSingleArtistMode,
            GenreMode
        };
        Q_ENUM(Mode)

        enum QueryField
        {
            IdField,
            FilePathField,
            TitleField,
            DurationField,
            DirectoryMediaArtField,
            EmbeddedMediaArtField,
            ArtistField,
            AlbumField
        };

        using SortMode = TracksModelSortMode::Mode;
        using InsideAlbumSortMode = TracksModelInsideAlbumSortMode::Mode;

        ~TracksModel() override;
        void classBegin() override;
        void componentComplete() override;

        QVariant data(const QModelIndex& index, int role) const override;

        Mode mode() const;
        void setMode(Mode mode);

        int artistId() const;
        void setArtistId(int id);

        int albumId() const;
        void setAlbumId(int id);

        int genreId() const;
        void setGenreId(int id);

        bool sortDescending() const;
        void setSortDescending(bool descending);

        SortMode sortMode() const;
        void setSortMode(SortMode mode);

        InsideAlbumSortMode insideAlbumSortMode() const;
        void setInsideAlbumSortMode(InsideAlbumSortMode mode);

        bool isRemovingFiles() const;

        Q_INVOKABLE std::vector<unplayer::LibraryTrack> getTracks(const std::vector<int>& indexes) const;
        Q_INVOKABLE unplayer::LibraryTrack getTrack(int index) const;
        Q_INVOKABLE QStringList getTrackPaths(const std::vector<int>& indexes) const;

        Q_INVOKABLE void removeTrack(int index, bool deleteFile);
        Q_INVOKABLE void removeTracks(const std::vector<int>& indexes, bool deleteFiles);

        static QString makeQueryString(Mode mode,
                                       SortMode sortMode,
                                       InsideAlbumSortMode insideAlbumSortMode,
                                       bool sortDescending,
                                       int artistId,
                                       int albumId,
                                       int genreId,
                                       bool& groupTracks);
        static LibraryTrack trackFromQuery(const QSqlQuery& query, bool groupTracks);

    protected:
        QHash<int, QByteArray> roleNames() const override;

        QString makeQueryString() override;
        LibraryTrack itemFromQuery(const QSqlQuery& query) override;

    private:
        std::vector<LibraryTrack>& mTracks = mItems;

        Mode mMode = AllTracksMode;
        int mArtistId = 0;
        int mAlbumId = 0;
        int mGenreId = 0;

        bool mSortDescending = false;
        SortMode mSortMode = SortMode::ArtistAlbumYear;
        InsideAlbumSortMode mInsideAlbumSortMode = InsideAlbumSortMode::DiscNumberTrackNumber;

        bool mGroupTracks;

    signals:
        void sortModeChanged();
        void insideAlbumSortModeChanged();
    };
}

#endif // UNPLAYER_TRACKSMODEL_H
