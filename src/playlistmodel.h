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

#ifndef UNPLAYER_PLAYLISTMODEL_H
#define UNPLAYER_PLAYLISTMODEL_H

#include <vector>
#include <QAbstractListModel>

#include "playlistutils.h"

namespace unplayer
{
    class PlaylistModel : public QAbstractListModel
    {
        Q_OBJECT
        Q_PROPERTY(QString filePath READ filePath WRITE setFilePath)
    public:
        enum Role
        {
            FilePathRole = Qt::UserRole,
            TitleRole,
            DurationRole,
            HasDurationRole,
            ArtistRole,
            AlbumRole,
            InLibraryRole
        };
        Q_ENUM(Role)

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& parent) const override;

        const QString& filePath() const;
        void setFilePath(const QString& filePath);

        Q_INVOKABLE QStringList getTracks(const std::vector<int>& indexes);
        Q_INVOKABLE void removeTrack(int index);
        Q_INVOKABLE void removeTracks(std::vector<int> indexes);
    protected:
        QHash<int, QByteArray> roleNames() const override;

    private:
        std::vector<PlaylistTrack> mTracks;
        QString mFilePath;
    };
}

#endif // UNPLAYER_PLAYLISTMODEL_H
