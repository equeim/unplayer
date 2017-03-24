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

#include <QDir>
#include <QItemSelectionModel>
#include <QUrl>

#include <QSparqlConnection>
#include <QSparqlQuery>
#include <QSparqlResult>

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

    struct DirectoryTrackFile
    {
        QString url;
        QString fileName;
        bool isDirectory;

        QString title;
        QString artist;
        bool unknownArtist;
        QString album;
        bool unknownAlbum;
        qint64 duration;
    };

    DirectoryTracksModel::DirectoryTracksModel()
        : mRowCount(0),
          mSparqlConnection(new QSparqlConnection(QStringLiteral("QTRACKER_DIRECT"), QSparqlConnectionOptions(), this)),
          mResult(nullptr),
          mLoaded(false),
          mTracksCount(0)
    {
    }

    DirectoryTracksModel::~DirectoryTracksModel()
    {
        if (mResult) {
            mResult->deleteLater();
        }
        qDeleteAll(mFiles);
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
        if (!index.isValid())
            return QVariant();

        const DirectoryTrackFile* file = mFiles.at(index.row());

        switch (role) {
        case UrlRole:
            return file->url;
        case FileNameRole:
            return file->fileName;
        case IsDirectoryRole:
            return file->isDirectory;
        case TitleRole:
            return file->title;
        case ArtistRole:
            return file->artist;
        case AlbumRole:
            return file->album;
        case DurationRole:
            return file->duration;
        default:
            return QVariant();
        }
    }

    int DirectoryTracksModel::rowCount(const QModelIndex&) const
    {
        return mRowCount;
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
        const DirectoryTrackFile* track = mFiles.at(fileIndex);

        QVariantMap map{{QStringLiteral("title"), track->title},
                        {QStringLiteral("url"), track->url},
                        {QStringLiteral("artist"), track->artist},
                        {QStringLiteral("album"), track->album},
                        {QStringLiteral("duration"), track->duration}};

        if (!track->unknownArtist) {
            map.insert(QStringLiteral("rawArtist"), track->artist);
        }

        if (!track->unknownAlbum) {
            map.insert(QStringLiteral("rawAlbum"), track->album);
        }

        return map;
    }

    QHash<int, QByteArray> DirectoryTracksModel::roleNames() const
    {
        return {{UrlRole, QByteArrayLiteral("url")},
                {FileNameRole, QByteArrayLiteral("fileName")},
                {IsDirectoryRole, QByteArrayLiteral("isDirectory")},
                {TitleRole, QByteArrayLiteral("title")},
                {ArtistRole, QByteArrayLiteral("artist")},
                {AlbumRole, QByteArrayLiteral("album")},
                {DurationRole, QByteArrayLiteral("duration")}};
    }

    void DirectoryTracksModel::loadDirectory()
    {
        mLoaded = false;
        mTracksCount = 0;
        emit loadedChanged();

        beginResetModel();
        qDeleteAll(mFiles);
        mFiles.clear();
        mRowCount = 0;
        endResetModel();

        mResult = mSparqlConnection->exec(QSparqlQuery(QStringLiteral("SELECT nie:url(?track) AS ?url\n"
                                                                      "       nfo:fileName(?track) AS ?fileName\n"
                                                                      "       tracker:coalesce(nie:title(?track), nfo:fileName(?track)) AS ?title\n"
                                                                      "       tracker:coalesce(nmm:artistName(nmm:performer(?track)), \"%1\") AS ?artist\n"
                                                                      "       nmm:artistName(nmm:performer(?track)) AS ?rawArtist\n"
                                                                      "       tracker:coalesce(nie:title(nmm:musicAlbum(?track)), \"%2\") AS ?album\n"
                                                                      "       nie:title(nmm:musicAlbum(?track)) AS ?rawAlbum\n"
                                                                      "       nie:informationElementDate(?track) AS ?date\n"
                                                                      "       nfo:duration(?track) AS ?duration\n"
                                                                      "WHERE {\n"
                                                                      "    ?track a nmm:MusicPiece;\n"
                                                                      "    nie:isPartOf [ nie:url \"%3\" ].\n"
                                                                      "}\n"
                                                                      "ORDER BY ?fileName")
                                                           .arg(tr("Unknown artist"))
                                                           .arg(tr("Unknown album"))
                                                           .arg(Utils::encodeUrl(QUrl::fromLocalFile(mDirectory)))));

        QObject::connect(mResult, &QSparqlResult::finished, this, &DirectoryTracksModel::onQueryFinished);
    }

    void DirectoryTracksModel::onQueryFinished()
    {
        for (const QFileInfo& info : QDir(mDirectory).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            mFiles.append(new DirectoryTrackFile{
                QUrl::fromLocalFile(info.filePath()).toString(),
                info.fileName(),
                true});
        }

        while (mResult->next()) {
            QSparqlResultRow row(mResult->current());
            mFiles.append(new DirectoryTrackFile{
                row.value(QStringLiteral("url")).toString(),
                row.value(QStringLiteral("fileName")).toString(),
                false,
                row.value(QStringLiteral("title")).toString(),
                row.value(QStringLiteral("artist")).toString(),
                !row.value(QStringLiteral("rawArtist")).isValid(),
                row.value(QStringLiteral("album")).toString(),
                !row.value(QStringLiteral("rawAlbum")).isValid(),
                row.value(QStringLiteral("duration")).toLongLong()});
            mTracksCount++;
        }

        beginResetModel();
        mRowCount = mFiles.size();
        endResetModel();

        mLoaded = true;
        emit loadedChanged();

        mResult->deleteLater();
        mResult = nullptr;
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
