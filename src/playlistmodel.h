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

#ifndef UNPLAYER_PLAYLISTMODEL_H
#define UNPLAYER_PLAYLISTMODEL_H

#include <vector>

#include <QString>

#include "asyncloadingmodel.h"
#include "playlistutils.h"

namespace unplayer
{
    class PlaylistModel : public AsyncLoadingModel
    {
        Q_OBJECT
        Q_PROPERTY(QString filePath READ filePath WRITE setFilePath)
    public:
        enum Role
        {
            UrlRole = Qt::UserRole,
            IsLocalFileRole,
            FilePathRole,
            TitleRole,
            DurationRole,
            ArtistRole,
            AlbumRole
        };
        Q_ENUM(Role)

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

        const QString& filePath() const;
        void setFilePath(const QString& playlistFilePath);

        Q_INVOKABLE QStringList getTracks(const std::vector<int>& indexes);
        Q_INVOKABLE void removeTrack(int index);
        Q_INVOKABLE void removeTracks(const std::vector<int>& indexes);
    protected:
        QHash<int, QByteArray> roleNames() const override;

    private:
        std::vector<PlaylistTrack> mTracks;
        QString mFilePath;
    };
}

#endif // UNPLAYER_PLAYLISTMODEL_H
