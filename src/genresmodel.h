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

#ifndef UNPLAYER_GENRESMODEL_H
#define UNPLAYER_GENRESMODEL_H

#include <vector>
#include <QAbstractListModel>

#include "librarytrack.h"

namespace unplayer
{
    struct Genre
    {
        QString genre;
        int tracksCount;
        int duration;
    };

    class GenresModel : public QAbstractListModel
    {
        Q_OBJECT
        Q_PROPERTY(bool sortDescending READ sortDescending NOTIFY sortDescendingChanged)
    public:
        GenresModel();

        QVariant data(const QModelIndex& index, int role) const override;
        int rowCount(const QModelIndex& parent) const override;

        bool sortDescending() const;
        Q_INVOKABLE void toggleSortOrder();

        Q_INVOKABLE std::vector<unplayer::LibraryTrack> getTracksForGenre(int index) const;
        Q_INVOKABLE std::vector<unplayer::LibraryTrack> getTracksForGenres(const std::vector<int>& indexes) const;
        Q_INVOKABLE QStringList getTrackPathsForGenre(int index) const;
        Q_INVOKABLE QStringList getTrackPathsForGenres(const std::vector<int>& indexes) const;

        Q_INVOKABLE void removeGenre(int index, bool deleteFiles);
        Q_INVOKABLE void removeGenres(std::vector<int> indexes, bool deleteFiles);
    protected:
        QHash<int, QByteArray> roleNames() const override;

    private:
        void execQuery();

        std::vector<Genre> mGenres;
        bool mSortDescending;

    signals:
        void sortDescendingChanged();
    };
}

#endif // UNPLAYER_GENRESMODEL_H
