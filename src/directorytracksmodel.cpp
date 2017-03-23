/*
 * Unplayer
 * Copyright (C) 2015 Alexey Rochev <equeim@gmail.com>
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

namespace unplayer
{

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
    : m_rowCount(0),
      m_sparqlConnection(new QSparqlConnection("QTRACKER_DIRECT", QSparqlConnectionOptions(), this)),
      m_result(nullptr),
      m_loaded(false),
      m_tracksCount(0)
{

}

DirectoryTracksModel::~DirectoryTracksModel()
{
    if (m_result != nullptr)
        m_result->deleteLater();
    qDeleteAll(m_files);
}

void DirectoryTracksModel::classBegin()
{

}

void DirectoryTracksModel::componentComplete()
{
    setDirectory(Utils::homeDirectory());
}

QVariant DirectoryTracksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const DirectoryTrackFile *file = m_files.at(index.row());

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
    return m_rowCount;
}

QString DirectoryTracksModel::directory() const
{
    return m_directory;
}

void DirectoryTracksModel::setDirectory(QString newDirectory)
{
    newDirectory = QDir(newDirectory).absolutePath();
    if (newDirectory != m_directory) {
        m_directory = newDirectory;
        emit directoryChanged();
        loadDirectory();
    }
}

QString DirectoryTracksModel::parentDirectory() const
{
    return QFileInfo(m_directory).path();
}

bool DirectoryTracksModel::isLoaded() const
{
    return m_loaded;
}

int DirectoryTracksModel::tracksCount() const
{
    return m_tracksCount;
}

QVariantMap DirectoryTracksModel::get(int fileIndex) const
{
    const DirectoryTrackFile *track = m_files.at(fileIndex);
    QVariantMap map;

    map.insert("title", track->title);
    map.insert("url", track->url);

    map.insert("artist", track->artist);
    if (track->unknownArtist)
        map.insert("rawArtist", QVariant());
    else
        map.insert("rawArtist", track->artist);

    map.insert("album", track->album);
    if (track->unknownAlbum)
        map.insert("rawAlbum", QVariant());
    else
        map.insert("rawAlbum", track->album);

    map.insert("duration", track->duration);

    return map;
}

QHash<int, QByteArray> DirectoryTracksModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles.insert(UrlRole, "url");
    roles.insert(FileNameRole, "fileName");
    roles.insert(IsDirectoryRole, "isDirectory");
    roles.insert(TitleRole, "title");
    roles.insert(ArtistRole, "artist");
    roles.insert(AlbumRole, "album");
    roles.insert(DurationRole, "duration");

    return roles;
}

void DirectoryTracksModel::loadDirectory()
{
    m_loaded = false;
    m_tracksCount = 0;
    emit loadedChanged();

    beginResetModel();
    qDeleteAll(m_files);
    m_files.clear();
    m_rowCount = 0;
    endResetModel();

    m_result = m_sparqlConnection->exec(QSparqlQuery(QString("SELECT nie:url(?track) AS ?url\n"
                                                             "       nfo:fileName(?track) AS ?fileName\n"
                                                             "       tracker:coalesce(nie:title(?track), nfo:fileName(?track)) AS ?title\n"
                                                             "       tracker:coalesce(nmm:artistName(nmm:performer(?track)), \"" + tr("Unknown artist") + "\") AS ?artist\n"
                                                             "       nmm:artistName(nmm:performer(?track)) AS ?rawArtist\n"
                                                             "       tracker:coalesce(nie:title(nmm:musicAlbum(?track)), \"" + tr("Unknown album") + "\") AS ?album\n"
                                                             "       nie:title(nmm:musicAlbum(?track)) AS ?rawAlbum\n"
                                                             "       nie:informationElementDate(?track) AS ?date\n"
                                                             "       nfo:duration(?track) AS ?duration\n"
                                                             "WHERE {\n"
                                                             "    ?track a nmm:MusicPiece;\n"
                                                             "    nie:isPartOf [ nie:url \"" + Utils::encodeUrl(QUrl::fromLocalFile(m_directory)) + "\" ].\n"
                                                             "}\n"
                                                             "ORDER BY ?fileName")));

    connect(m_result, &QSparqlResult::finished, this, &DirectoryTracksModel::onQueryFinished);
}

void DirectoryTracksModel::onQueryFinished()
{
    for (const QFileInfo &info : QDir(m_directory).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        m_files.append(new DirectoryTrackFile {
                           QUrl::fromLocalFile(info.filePath()).toString(),
                           info.fileName(),
                           true
                       });
    }

    while (m_result->next()) {
        QSparqlResultRow row(m_result->current());
        m_files.append(new DirectoryTrackFile {
                           row.value("url").toString(),
                           row.value("fileName").toString(),
                           false,
                           row.value("title").toString(),
                           row.value("artist").toString(),
                           !row.value("rawArtist").isValid(),
                           row.value("album").toString(),
                           !row.value("rawAlbum").isValid(),
                           row.value("duration").toLongLong()
                       });
        m_tracksCount++;
    }

    beginResetModel();
    m_rowCount = m_files.size();
    endResetModel();

    m_loaded = true;
    emit loadedChanged();

    m_result->deleteLater();
    m_result = nullptr;
}

void DirectoryTracksProxyModel::componentComplete()
{
    connect(this, &QAbstractItemModel::modelReset, this, &DirectoryTracksProxyModel::tracksCountChanged);
    connect(this, &QAbstractItemModel::rowsInserted, this, &DirectoryTracksProxyModel::tracksCountChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &DirectoryTracksProxyModel::tracksCountChanged);
    FilterProxyModel::componentComplete();
}

int DirectoryTracksProxyModel::tracksCount() const
{
    int count = 0;
    for (int i = 0, max = rowCount(); i < max; i++) {
        if (!index(i, 0).data(IsDirectoryRole).toBool())
            count++;
    }
    return count;
}

QVariantList DirectoryTracksProxyModel::getTracks() const
{
    QVariantList tracks;
    DirectoryTracksModel *model = static_cast<DirectoryTracksModel*>(sourceModel());
    for (int i = 0, max = rowCount(); i < max; i++) {
        if (!index(i, 0).data(IsDirectoryRole).toBool())
            tracks.append(model->get(sourceIndex(i)));
    }
    return tracks;
}

QVariantList DirectoryTracksProxyModel::getSelectedTracks() const
{
    QVariantList tracks;
    DirectoryTracksModel *model = static_cast<DirectoryTracksModel*>(sourceModel());

    for (int index : selectedSourceIndexes())
        tracks.append(model->get(index));

    return tracks;
}

void DirectoryTracksProxyModel::selectAll()
{
    for (int i = 0, max = rowCount(); i < max; i++) {
        QModelIndex modelIndex(index(i, 0));
        if (!modelIndex.data(IsDirectoryRole).toBool())
            selectionModel()->select(modelIndex, QItemSelectionModel::Select);
    }
}

}
