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

#include "albumsmodel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFutureWatcher>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtConcurrentRun>

#include "libraryutils.h"
#include "settings.h"
#include "utils.h"

namespace unplayer
{
    namespace
    {
        enum Field
        {
            ArtistField,
            AlbumField,
            YearField,
            TracksCountField,
            DurationField
        };
    }

    AlbumsModel::~AlbumsModel()
    {
        if (mAllArtists) {
            Settings::instance()->setAllAlbumsSortSettings(mSortDescending, mSortMode);
        } else {
            Settings::instance()->setAlbumsSortSettings(mSortDescending, mSortMode);
        }
    }

    void AlbumsModel::classBegin()
    {

    }

    void AlbumsModel::componentComplete()
    {
        if (mAllArtists) {
            mSortDescending = Settings::instance()->allAlbumsSortDescending();
            mSortMode = static_cast<SortMode>(Settings::instance()->allAlbumsSortMode(SortArtistYear));
        } else {
            mSortDescending = Settings::instance()->albumsSortDescending();
            mSortMode = static_cast<SortMode>(Settings::instance()->albumsSortMode(SortYear));
        }

        emit sortModeChanged();

        execQuery();
    }

    QVariant AlbumsModel::data(const QModelIndex& index, int role) const
    {
        const Album& album = mAlbums[index.row()];

        switch (role) {
        case ArtistRole:
            return album.artist;
        case DisplayedArtistRole:
            return album.displayedArtist;
        case UnknownArtistRole:
            return album.artist.isEmpty();
        case AlbumRole:
            return album.album;
        case DisplayedAlbumRole:
            return album.displayedAlbum;
        case UnknownAlbumRole:
            return album.album.isEmpty();
        case YearRole:
            return album.year;
        case TracksCountRole:
            return album.tracksCount;
        case DurationRole:
            return album.duration;
        default:
            return QVariant();
        }
    }

    int AlbumsModel::rowCount(const QModelIndex&) const
    {
        return mAlbums.size();
    }

    bool AlbumsModel::allArtists() const
    {
        return mAllArtists;
    }

    void AlbumsModel::setAllArtists(bool allArtists)
    {
        mAllArtists = allArtists;
        emit allArtistsChanged();
    }

    const QString& AlbumsModel::artist() const
    {
        return mArtist;
    }

    void AlbumsModel::setArtist(const QString& artist)
    {
        mArtist = artist;
    }

    bool AlbumsModel::sortDescending() const
    {
        return mSortDescending;
    }

    void AlbumsModel::setSortDescending(bool descending)
    {
        if (descending != mSortDescending) {
            mSortDescending = descending;
            execQuery();
        }
    }

    AlbumsModel::SortMode AlbumsModel::sortMode() const
    {
        return mSortMode;
    }

    void AlbumsModel::setSortMode(AlbumsModel::SortMode mode)
    {
        if (mode != mSortMode) {
            mSortMode = mode;
            emit sortModeChanged();
            execQuery();
        }
    }

    bool AlbumsModel::isRemovingFiles() const
    {
        return mRemovingFiles;
    }

    QStringList AlbumsModel::getTracksForAlbum(int index) const
    {
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT filePath FROM tracks "
                                     "WHERE artist = ? AND album = ? "
                                     "ORDER BY trackNumber, title"));
        const Album& album = mAlbums[index];
        query.addBindValue(album.artist);
        query.addBindValue(album.album);
        if (query.exec()) {
            QStringList tracks;
            while (query.next()) {
                tracks.append(query.value(0).toString());
            }
            return tracks;
        }

        qWarning() << "failed to get tracks from database" << query.lastError();
        return QStringList();
    }

    QStringList AlbumsModel::getTracksForAlbums(const std::vector<int>& indexes) const
    {
        QStringList tracks;
        QSqlDatabase::database().transaction();
        for (int index : indexes) {
            tracks.append(getTracksForAlbum(index));
        }
        QSqlDatabase::database().commit();
        return tracks;
    }

    void AlbumsModel::removeAlbum(int index, bool deleteFiles)
    {
        removeAlbums({index}, deleteFiles);
    }

    void AlbumsModel::removeAlbums(std::vector<int> indexes, bool deleteFiles)
    {
        if (mRemovingFiles) {
            return;
        }

        mRemovingFiles = true;
        emit removingFilesChanged();

        auto future = QtConcurrent::run(std::bind([this, deleteFiles](std::vector<int>& indexes) {
            std::reverse(indexes.begin(), indexes.end());
            std::vector<int> removed;
            {
                auto db = QSqlDatabase::addDatabase(LibraryUtils::databaseType, staticMetaObject.className());
                db.setDatabaseName(LibraryUtils::instance()->databaseFilePath());
                if (!db.open()) {
                    QSqlDatabase::removeDatabase(staticMetaObject.className());
                    qWarning() << "failed to open database" << db.lastError();
                    return removed;
                }
                db.transaction();

                for (int index : indexes) {
                    const QString artist(mAlbums[index].artist);
                    const QString album(mAlbums[index].album);

                    if (deleteFiles) {
                        QSqlQuery query(db);
                        query.prepare(QStringLiteral("SELECT DISTINCT filePath FROM tracks WHERE artist = ? AND album = ?"));
                        query.addBindValue(artist);
                        query.addBindValue(album);
                        query.exec();
                        while (query.next()) {
                            const QString filePath(query.value(0).toString());
                            if (!QFile::remove(filePath)) {
                                qWarning() << "failed to remove file:" << filePath;
                            }
                        }
                    }

                    QSqlQuery query(db);
                    query.prepare(QStringLiteral("DELETE FROM tracks WHERE artist = ? AND album = ?"));
                    query.addBindValue(artist);
                    query.addBindValue(album);
                    if (query.exec()) {
                        removed.push_back(index);
                    } else {
                        qWarning() << "failed to remove files from database" << query.lastQuery() << query.lastError();
                    }
                }
                db.commit();
            }
            QSqlDatabase::removeDatabase(staticMetaObject.className());

            return removed;
        }, std::move(indexes)));

        using Watcher = QFutureWatcher<std::vector<int>>;
        auto watcher = new Watcher(this);
        QObject::connect(watcher, &Watcher::finished, this, [this, watcher]() {
            mRemovingFiles = false;
            emit removingFilesChanged();

            for (int index : watcher->result()) {
                beginRemoveRows(QModelIndex(), index, index);
                mAlbums.erase(mAlbums.begin() + index);
                endRemoveRows();
            }
            emit LibraryUtils::instance()->databaseChanged();
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }

    QHash<int, QByteArray> AlbumsModel::roleNames() const
    {
        return {{ArtistRole, "artist"},
                {DisplayedArtistRole, "displayedArtist"},
                {UnknownArtistRole, "unknownArtist"},
                {AlbumRole, "album"},
                {DisplayedAlbumRole, "displayedAlbum"},
                {UnknownAlbumRole, "unknownAlbum"},
                {YearRole, "year"},
                {TracksCountRole, "tracksCount"},
            {DurationRole, "duration"}};
    }

    void AlbumsModel::execQuery()
    {
        QString queryString(QLatin1String("SELECT artist, album, year, COUNT(*), SUM(duration) FROM "
                                          "(SELECT artist, album, year, duration FROM tracks GROUP BY id, artist, album) "));
        if (mAllArtists) {
            queryString += QLatin1String("GROUP BY album, artist ");
        } else {
            queryString += QLatin1String("WHERE artist = ? "
                                   "GROUP BY album ");
        }

        switch (mSortMode) {
        case SortAlbum:
            queryString += QLatin1String("ORDER BY album = '' %1, album %1");
            break;
        case SortYear:
            queryString += QLatin1String("ORDER BY year %1, album = '' %1, album %1");
            break;
        case SortArtistAlbum:
            queryString += QLatin1String("ORDER BY artist = '' %1, artist %1, album = '' %1, album %1");
            break;
        case SortArtistYear:
            queryString += QLatin1String("ORDER BY artist = '' %1, artist %1, year %1, album = '' %1, album %1");
        }

        queryString = queryString.arg(mSortDescending ? QLatin1String("DESC")
                                                      : QLatin1String("ASC"));


        QSqlQuery query;
        query.prepare(queryString);
        if (!mAllArtists) {
            query.addBindValue(mArtist);
        }

        beginResetModel();
        mAlbums.clear();
        if (query.exec()) {
            while (query.next()) {
                const QString artist(query.value(ArtistField).toString());
                const QString album(query.value(AlbumField).toString());
                mAlbums.push_back({artist,
                                   artist.isEmpty() ? qApp->translate("unplayer", "Unknown artist") : artist,
                                   album,
                                   album.isEmpty() ? qApp->translate("unplayer", "Unknown album") : album,
                                   query.value(YearField).toInt(),
                                   query.value(TracksCountField).toInt(),
                                   query.value(DurationField).toInt()});
            }
        } else {
            qWarning() << query.lastError();
        }
        endResetModel();
    }
}
