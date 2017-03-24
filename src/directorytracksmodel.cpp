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

#include "directorytracksmodel.h"

#include <QDebug>
#include <QDir>
#include <QFutureWatcher>
#include <QItemSelectionModel>
#include <QUrl>
#include <QtConcurrentRun>

#include <fileref.h>
#include <tag.h>

#include "utils.h"

namespace unplayer
{
    namespace
    {
        enum DirectoryTracksModelRole
        {
            UrlRole = Qt::UserRole,
            FileNameRole,
            IsDirectoryRole,
            TitleRole,
            ArtistRole,
            AlbumRole,
            DurationRole
        };
    }

    DirectoryTracksModel::DirectoryTracksModel()
        : mLoaded(false),
          mTracksCount(0)
    {
    }

    void DirectoryTracksModel::classBegin()
    {
    }

    void DirectoryTracksModel::componentComplete()
    {
        setDirectory(Utils::homeDirectory());
    }

    QVariant DirectoryTracksModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid()) {
            return QVariant();
        }

        const DirectoryTrackFile& file = mFiles.at(index.row());

        switch (role) {
        case UrlRole:
            return file.url;
        case FileNameRole:
            return file.fileName;
        case IsDirectoryRole:
            return file.isDirectory;
        default:
            return QVariant();
        }
    }

    int DirectoryTracksModel::rowCount(const QModelIndex&) const
    {
        return mFiles.size();
    }

    QString DirectoryTracksModel::directory() const
    {
        return mDirectory;
    }

    void DirectoryTracksModel::setDirectory(QString newDirectory)
    {
        newDirectory = QDir(newDirectory).absolutePath();
        if (newDirectory != mDirectory) {
            mDirectory = newDirectory;
            emit directoryChanged();
            loadDirectory();
        }
    }

    QString DirectoryTracksModel::parentDirectory() const
    {
        return QFileInfo(mDirectory).path();
    }

    bool DirectoryTracksModel::isLoaded() const
    {
        return mLoaded;
    }

    int DirectoryTracksModel::tracksCount() const
    {
        return mTracksCount;
    }

    QVariantMap DirectoryTracksModel::get(int fileIndex) const
    {
        const DirectoryTrackFile& track = mFiles.at(fileIndex);

        QVariantMap map{{QStringLiteral("title"), track.title},
                        {QStringLiteral("url"), track.url},
                        {QStringLiteral("artist"), track.artist},
                        {QStringLiteral("album"), track.album},
                        {QStringLiteral("duration"), track.duration}};

        if (!track.unknownArtist) {
            map.insert(QStringLiteral("rawArtist"), track.artist);
        }

        if (!track.unknownAlbum) {
            map.insert(QStringLiteral("rawAlbum"), track.album);
        }

        return map;
    }

    QHash<int, QByteArray> DirectoryTracksModel::roleNames() const
    {
        return {{UrlRole, QByteArrayLiteral("url")},
                {FileNameRole, QByteArrayLiteral("fileName")},
                {IsDirectoryRole, QByteArrayLiteral("isDirectory")}};
    }

    void DirectoryTracksModel::loadDirectory()
    {
        mLoaded = false;
        mTracksCount = 0;
        emit loadedChanged();

        beginResetModel();
        mFiles.clear();

        const QString directory = mDirectory;
        const auto future = QtConcurrent::run([directory]() {
            QList<DirectoryTrackFile> files;
            int tracksCount = 0;

            const QDir dir(directory);

            for (const QFileInfo& info : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                files.append(DirectoryTrackFile{QUrl::fromLocalFile(info.filePath()).toString(),
                                                 info.fileName(),
                                                 true});
            }

            const QMimeDatabase mimeDb;
            for (const QFileInfo& info : dir.entryInfoList(QDir::Files)) {
                if (mimeDb.mimeTypeForFile(info, QMimeDatabase::MatchExtension).name().startsWith(QLatin1String("audio/"))) {
                    DirectoryTrackFile file{QUrl::fromLocalFile(info.filePath()).toString(),
                                            info.fileName(),
                                            false};

                    TagLib::FileRef tagFile(info.filePath().toUtf8().constData());

                    if (tagFile.tag()) {
                        const TagLib::Tag* tag = tagFile.tag();

                        file.title = tag->title().toCString();
                        if (file.title.isEmpty()) {
                            file.title = file.fileName;
                        }

                        file.artist = tag->artist().toCString();
                        file.unknownArtist = (file.artist.isEmpty());
                        if (file.unknownArtist) {
                            file.artist = tr("Unknown artist");
                        }

                        file.album = tag->album().toCString();
                        file.unknownAlbum = (file.album.isEmpty());
                        if (file.unknownAlbum) {
                            file.album = tr("Unknown album");
                        }
                    }

                    if (tagFile.audioProperties()) {
                        file.duration = tagFile.audioProperties()->length();
                    }

                    files.append(file);

                    tracksCount++;
                }
            }

            return std::pair<QList<DirectoryTrackFile>, int>(files, tracksCount);
        });

        using FutureWatcher = QFutureWatcher<std::pair<QList<DirectoryTrackFile>, int>>;
        auto watcher = new FutureWatcher(this);
        QObject::connect(watcher, &FutureWatcher::finished, this, [=]() {
            const auto result = watcher->result();
            mFiles = result.first;
            endResetModel();
            mTracksCount = result.second;
            mLoaded = true;
            emit loadedChanged();
        });
        watcher->setFuture(future);
    }

    void DirectoryTracksProxyModel::componentComplete()
    {
        QObject::connect(this, &QAbstractItemModel::modelReset, this, &DirectoryTracksProxyModel::tracksCountChanged);
        QObject::connect(this, &QAbstractItemModel::rowsInserted, this, &DirectoryTracksProxyModel::tracksCountChanged);
        QObject::connect(this, &QAbstractItemModel::rowsRemoved, this, &DirectoryTracksProxyModel::tracksCountChanged);
        FilterProxyModel::componentComplete();
    }

    int DirectoryTracksProxyModel::tracksCount() const
    {
        int count = 0;
        for (int i = 0, max = rowCount(); i < max; i++) {
            if (!index(i, 0).data(IsDirectoryRole).toBool()) {
                count++;
            }
        }
        return count;
    }

    QVariantList DirectoryTracksProxyModel::getTracks() const
    {
        QVariantList tracks;
        DirectoryTracksModel* model = static_cast<DirectoryTracksModel*>(sourceModel());
        for (int i = 0, max = rowCount(); i < max; i++) {
            if (!index(i, 0).data(IsDirectoryRole).toBool()) {
                tracks.append(model->get(sourceIndex(i)));
            }
        }
        return tracks;
    }

    QVariantList DirectoryTracksProxyModel::getSelectedTracks() const
    {
        QVariantList tracks;
        DirectoryTracksModel* model = static_cast<DirectoryTracksModel*>(sourceModel());

        for (int index : selectedSourceIndexes()) {
            tracks.append(model->get(index));
        }

        return tracks;
    }

    void DirectoryTracksProxyModel::selectAll()
    {
        for (int i = 0, max = rowCount(); i < max; i++) {
            QModelIndex modelIndex(index(i, 0));
            if (!modelIndex.data(IsDirectoryRole).toBool()) {
                selectionModel()->select(modelIndex, QItemSelectionModel::Select);
            }
        }
    }
}
