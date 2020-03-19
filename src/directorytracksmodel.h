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

#ifndef UNPLAYER_DIRECTORYTRACKSMODEL_H
#define UNPLAYER_DIRECTORYTRACKSMODEL_H

#include <vector>
#include <QAbstractListModel>

#include "directorycontentproxymodel.h"

namespace unplayer
{
    struct DirectoryTrackFile
    {
        QString filePath;
        QString fileName;
        bool isDirectory;
        bool isPlaylist;
    };

    class DirectoryTracksModel : public QAbstractListModel, public QQmlParserStatus
    {
        Q_OBJECT
        Q_INTERFACES(QQmlParserStatus)
        Q_PROPERTY(QString directory READ directory WRITE setDirectory NOTIFY directoryChanged)
        Q_PROPERTY(QString parentDirectory READ parentDirectory NOTIFY directoryChanged)
        Q_PROPERTY(bool loaded READ isLoaded NOTIFY loadedChanged)
    public:
        enum Role
        {
            FilePathRole = Qt::UserRole,
            FileNameRole,
            IsDirectoryRole,
            IsPlaylistRole,
        };
        Q_ENUM(Role)

        void classBegin() override;
        void componentComplete() override;

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& parent) const override;

        const std::vector<DirectoryTrackFile>& files() const;

        QString directory() const;
        void setDirectory(QString newDirectory);

        QString parentDirectory() const;
        bool isLoaded() const;

        Q_INVOKABLE QString getTrack(int index) const;
        Q_INVOKABLE QStringList getTracks(const std::vector<int>& indexes, bool includePlaylists = true) const;

        Q_INVOKABLE void removeTrack(int index);
        Q_INVOKABLE void removeTracks(const std::vector<int>& indexes);

    protected:
        QHash<int, QByteArray> roleNames() const override;

    private:
        void loadDirectory();
        void onQueryFinished();

    private:
        std::vector<DirectoryTrackFile> mFiles;

        int mTracksCount = 0;

        QString mDirectory;
        bool mLoaded = false;

        bool mShowVideoFiles = false;
    signals:
        void directoryChanged();
        void loadedChanged();
    };

    class DirectoryTracksProxyModel : public DirectoryContentProxyModel
    {
        Q_OBJECT
        Q_PROPERTY(int directoriesCount READ directoriesCount NOTIFY countChanged)
        Q_PROPERTY(int tracksCount READ tracksCount NOTIFY countChanged)
    public:
        DirectoryTracksProxyModel();
        void componentComplete() override;

        int directoriesCount() const;
        int tracksCount() const;

        Q_INVOKABLE QStringList getSelectedTracks() const;
        Q_INVOKABLE void selectAll();

    private:
        int mDirectoriesCount;
        int mTracksCount;
    signals:
        void countChanged();
    };
}

//Q_DECLARE_TYPEINFO(unplayer::DirectoryTrackFile, Q_MOVABLE_TYPE);

#endif // UNPLAYER_DIRECTORYTRACKSMODEL_H
