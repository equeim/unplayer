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

#include <QQmlParserStatus>

#include "abstractlibrarymodel.h"
#include "librarytrack.h"

namespace unplayer
{
    struct TracksModelSortMode
    {
        Q_GADGET
    public:
        // Not used when QueryMode is QueryAlbumTracksForAllArtists or QueryAlbumTracksForSingleArtist
        enum Mode
        {
            Title,
            AddedDate,
            Artist_AlbumTitle,
            Artist_AlbumYear
        };
        Q_ENUM(Mode)
        static Mode fromInt(int value);
        static Mode fromSettingsForAllTracks();
        static Mode fromSettingsForArtist();
    };

    struct TracksModelInsideAlbumSortMode
    {
        Q_GADGET
    public:
        // Only used when TracksModelSortMode is ArtistAlbumTitle or ArtistAlbumYear
        // or QueryMode is QueryAlbumTracksForAllArtists or QueryAlbumTracksForSingleArtist
        enum Mode
        {
            Title,
            DiscNumber_Title,
            DiscNumber_TrackNumber
        };
        Q_ENUM(Mode)

        static Mode fromInt(int value);
        static Mode fromSettingsForAllTracks();
        static Mode fromSettingsForArtist();
        static Mode fromSettingsForAlbum();
    };

    class TracksModel : public AbstractLibraryModel<LibraryTrack>, public QQmlParserStatus
    {
        Q_OBJECT

        Q_INTERFACES(QQmlParserStatus)

        Q_PROPERTY(QueryMode queryMode READ queryMode WRITE setQueryMode NOTIFY queryModeChanged)
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

        enum QueryMode
        {
            QueryAllTracks,
            QueryArtistTracks,
            QueryAlbumTracksForAllArtists,
            QueryAlbumTracksForSingleArtist,
            QueryGenreTracks
        };
        Q_ENUM(QueryMode)

        enum QueryField
        {
            IdField,
            FilePathField,
            TitleField,
            DurationField,
            ArtistField,
            AlbumField
        };

        using SortMode = TracksModelSortMode::Mode;
        using InsideAlbumSortMode = TracksModelInsideAlbumSortMode::Mode;

        ~TracksModel() override;
        void classBegin() override;
        void componentComplete() override;

        QVariant data(const QModelIndex& index, int role) const override;

        QueryMode queryMode() const;
        void setQueryMode(QueryMode mode);

        // Artist id for QuerySingleArtistTracks and QueryAlbumTracksForSingleArtist query modes
        // Zero means query for unknown artist
        int artistId() const;
        void setArtistId(int id);

        // Album id for QueryAlbumTracksForAllArtists and QueryAlbumTracksForSingleArtist query modes
        // Zero means query for unknown album
        int albumId() const;
        void setAlbumId(int id);

        // Genre id for QueryGenreTracks query mode
        // Can't be zero
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

        static QString makeQueryString(QueryMode queryMode,
                                       SortMode sortMode,
                                       InsideAlbumSortMode insideAlbumSortMode,
                                       bool sortDescending,
                                       int artistId,
                                       int albumId,
                                       int genreId,
                                       bool& groupTracks);
        static LibraryTrack trackFromQuery(const QSqlQuery& query, bool groupTracks);

    protected:
        class ItemFactory final : public AbstractItemFactory
        {
        public:
            inline explicit ItemFactory(bool groupTracks) : groupTracks(groupTracks) {}
            LibraryTrack itemFromQuery(const QSqlQuery& query) override;
            bool groupTracks;
        };

        QHash<int, QByteArray> roleNames() const override;

        QString makeQueryString() override;
        AbstractItemFactory* createItemFactory() override;

    private:
        std::vector<LibraryTrack>& mTracks = mItems;

        QueryMode mQueryMode = QueryAllTracks;
        int mArtistId = 0;
        int mAlbumId = 0;
        int mGenreId = 0;

        bool mSortDescending = false;
        SortMode mSortMode{};
        InsideAlbumSortMode mInsideAlbumSortMode{};

        bool mGroupTracks;

    signals:
        void queryModeChanged(QueryMode queryMode);
        void sortModeChanged();
        void insideAlbumSortModeChanged();
    };
}

#endif // UNPLAYER_TRACKSMODEL_H
