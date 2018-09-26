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

#include "genresmodel.h"

#include <functional>

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
            GenreField,
            TracksCountField,
            DurationField
        };

        enum Role
        {
            GenreRole = Qt::UserRole,
            TracksCountRole,
            DurationRole
        };
    }

    GenresModel::GenresModel()
        : mSortDescending(Settings::instance()->genresSortDescending()),
          mRemovingFiles(false)
    {
        execQuery();
    }

    QVariant GenresModel::data(const QModelIndex& index, int role) const
    {
        const Genre& genre = mGenres[index.row()];
        switch (role) {
        case GenreRole:
            return genre.genre;
        case TracksCountRole:
            return genre.tracksCount;
        case DurationRole:
            return genre.duration;
        default:
            return QVariant();
        }
    }

    int GenresModel::rowCount(const QModelIndex&) const
    {
        return mGenres.size();
    }

    bool GenresModel::sortDescending() const
    {
        return mSortDescending;
    }

    void GenresModel::toggleSortOrder()
    {
        mSortDescending = !mSortDescending;
        Settings::instance()->setGenresSortDescending(mSortDescending);
        emit sortDescendingChanged();
        execQuery();
    }

    bool GenresModel::isRemovingFiles() const
    {
        return mRemovingFiles;
    }

    QStringList GenresModel::getTracksForGenre(int index) const
    {
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT filePath FROM tracks "
                                     "WHERE genre = ?  "
                                     "ORDER BY artist = '', artist, album = '', year, album, trackNumber, title"));
        query.addBindValue(mGenres[index].genre);
        if (query.exec()) {
            QStringList tracks;
            while (query.next()) {
                tracks.append(query.value(0).toString());
            }
            return tracks;
        }

        qWarning() << "failed to get tracks from database";
        return QStringList();
    }

    QStringList GenresModel::getTracksForGenres(const std::vector<int>& indexes) const
    {
        QStringList tracks;
        QSqlDatabase::database().transaction();
        for (int index : indexes) {
            tracks.append(getTracksForGenre(index));
        }
        QSqlDatabase::database().commit();
        return tracks;
    }

    void GenresModel::removeGenre(int index, bool deleteFiles)
    {
        removeGenres({index}, deleteFiles);
    }

    void GenresModel::removeGenres(std::vector<int> indexes, bool deleteFiles)
    {
        if (mRemovingFiles) {
            return;
        }

        mRemovingFiles = true;
        emit removingFilesChanged();

        auto future = QtConcurrent::run(std::bind([this, deleteFiles](std::vector<int>& indexes) {
            std::sort(indexes.begin(), indexes.end(), std::greater<int>());
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
                    const QString genre(mGenres[index].genre);

                    if (deleteFiles) {
                        QSqlQuery query(db);
                        query.prepare(QStringLiteral("SELECT DISTINCT filePath FROM tracks WHERE genre = ?"));
                        query.addBindValue(genre);
                        query.exec();
                        while (query.next()) {
                            const QString filePath(query.value(0).toString());
                            if (!QFile::remove(filePath)) {
                                qWarning() << "failed to remove file:" << filePath;
                            }
                        }
                    }

                    QSqlQuery query(db);
                    query.prepare(QStringLiteral("DELETE FROM tracks WHERE genre = ?"));
                    query.addBindValue(genre);
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
                mGenres.erase(mGenres.begin() + index);
                endRemoveRows();
            }
            emit LibraryUtils::instance()->databaseChanged();
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }

    QHash<int, QByteArray> GenresModel::roleNames() const
    {
        return {{GenreRole, "genre"},
                {TracksCountRole, "tracksCount"},
            {DurationRole, "duration"}};
    }

    void GenresModel::execQuery()
    {
        beginResetModel();
        mGenres.clear();
        QSqlQuery query(QString::fromLatin1("SELECT genre, COUNT(*), SUM(duration) FROM tracks "
                                            "WHERE genre != '' "
                                            "GROUP BY genre "
                                            "ORDER BY genre %1").arg(mSortDescending ? QLatin1String("DESC")
                                                                                     : QLatin1String("ASC")));

        if (query.lastError().type() == QSqlError::NoError) {
            while (query.next()) {
                mGenres.push_back({query.value(GenreField).toString(),
                                   query.value(TracksCountField).toInt(),
                                   query.value(DurationField).toInt()});
            }

        } else {
            qWarning() << query.lastError();
        }
        endResetModel();
    }
}
