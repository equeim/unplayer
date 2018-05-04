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

#include "artistsmodel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFutureWatcher>
#include <QSqlError>
#include <QSqlQuery>
#include <QtConcurrentRun>

#include "libraryutils.h"
#include "settings.h"

namespace unplayer
{
    namespace
    {
        enum Field
        {
            ArtistField,
            AlbumsCountField,
            TracksCountField,
            DurationField
        };
    }

    ArtistsModel::ArtistsModel()
        : mSortDescending(Settings::instance()->artistsSortDescending()),
          mRemovingFiles(false)
    {
        execQuery();
    }

    QVariant ArtistsModel::data(const QModelIndex& index, int role) const
    {
        const Artist& artist = mArtists[index.row()];

        switch (role) {
        case ArtistRole:
            return artist.artist;
        case DisplayedArtistRole:
            return artist.displayedArtist;
        case AlbumsCountRole:
            return artist.albumsCount;
        case TracksCountRole:
            return artist.tracksCount;
        case DurationRole:
            return artist.duration;
        default:
            return QVariant();
        }
    }

    int ArtistsModel::rowCount(const QModelIndex&) const
    {
        return mArtists.size();
    }

    bool ArtistsModel::sortDescending() const
    {
        return mSortDescending;
    }

    void ArtistsModel::toggleSortOrder()
    {
        mSortDescending = !mSortDescending;
        Settings::instance()->setArtistsSortDescending(mSortDescending);
        emit sortDescendingChanged();
        execQuery();
    }

    bool ArtistsModel::isRemovingFiles() const
    {
        return mRemovingFiles;
    }

    QStringList ArtistsModel::getTracksForArtist(int index) const
    {
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT filePath FROM tracks "
                                     "WHERE artist = ? "
                                     "ORDER BY album = '', year, album, trackNumber, title"));
        query.addBindValue(mArtists[index].artist);
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

    QStringList ArtistsModel::getTracksForArtists(const std::vector<int>& indexes) const
    {
        QStringList tracks;
        QSqlDatabase::database().transaction();
        for (int index : indexes) {
            tracks.append(getTracksForArtist(index));
        }
        QSqlDatabase::database().commit();
        return tracks;
    }

    void ArtistsModel::removeArtist(int index, bool deleteFiles)
    {
        removeArtists({index}, deleteFiles);
    }

    void ArtistsModel::removeArtists(std::vector<int> indexes, bool deleteFiles)
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
                    const QString artist(mArtists[index].artist);

                    if (deleteFiles) {
                        QSqlQuery query(db);
                        query.prepare(QStringLiteral("SELECT DISTINCT filePath FROM tracks WHERE artist = ?"));
                        query.addBindValue(artist);
                        query.exec();
                        while (query.next()) {
                            const QString filePath(query.value(0).toString());
                            if (!QFile::remove(filePath)) {
                                qWarning() << "failed to remove file:" << filePath;
                            }
                        }
                    }

                    QSqlQuery query(db);
                    query.prepare(QStringLiteral("DELETE FROM tracks WHERE artist = ?"));
                    query.addBindValue(artist);
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
                mArtists.erase(mArtists.begin() + index);
                endRemoveRows();
            }
            emit LibraryUtils::instance()->databaseChanged();
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }

    QHash<int, QByteArray> ArtistsModel::roleNames() const
    {
        return {{ArtistRole, "artist"},
                {DisplayedArtistRole, "displayedArtist"},
                {AlbumsCountRole, "albumsCount"},
                {TracksCountRole, "tracksCount"},
            {DurationRole, "duration"}};
    }

    void ArtistsModel::execQuery()
    {
        beginResetModel();
        mArtists.clear();
        QSqlQuery query(QString::fromLatin1("SELECT artist, COUNT(DISTINCT(album)), COUNT(*), SUM(duration) FROM "
                                            "(SELECT artist, album, duration FROM tracks GROUP BY id, artist, album) "
                                            "GROUP BY artist "
                                            "ORDER BY artist = '' %1, artist %1").arg(mSortDescending ? QLatin1String("DESC")
                                                                                                      : QLatin1String("ASC")));
        if (query.lastError().type() == QSqlError::NoError) {
            while (query.next()) {
                const QString artist(query.value(ArtistField).toString());
                mArtists.push_back({artist,
                                    artist.isEmpty() ? qApp->translate("unplayer", "Unknown artist") : artist,
                                    query.value(AlbumsCountField).toInt(),
                                    query.value(TracksCountField).toInt(),
                                    query.value(DurationField).toInt()});
            }

        } else {
            qWarning() << query.lastError();
        }
        endResetModel();
    }
}
