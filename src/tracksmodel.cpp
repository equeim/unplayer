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

#include "tracksmodel.h"

#include <functional>

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFutureWatcher>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
#include <QtConcurrentRun>

#include "libraryutils.h"
#include "settings.h"

namespace unplayer
{
    namespace
    {
        enum Field
        {
            FilePathField,
            TitleField,
            ArtistField,
            AlbumField,
            DurationField
        };
    }

    TracksModel::~TracksModel()
    {
        if (mAllArtists) {
            Settings::instance()->setAllTracksSortSettings(mSortDescending, mSortMode, mInsideAlbumSortMode);
        } else {
            if (mAllAlbums) {
                Settings::instance()->setArtistTracksSortSettings(mSortDescending, mSortMode, mInsideAlbumSortMode);
            } else {
                Settings::instance()->setAlbumTracksSortSettings(mSortDescending, mInsideAlbumSortMode);
            }
        }
    }

    void TracksModel::classBegin()
    {

    }

    void TracksModel::componentComplete()
    {
        if (mAllArtists) {
            mSortDescending = Settings::instance()->allTracksSortDescending();
            mSortMode = static_cast<SortMode>(Settings::instance()->allTracksSortMode(SortMode::ArtistAlbumYear));
            mInsideAlbumSortMode = static_cast<InsideAlbumSortMode>(Settings::instance()->allTracksInsideAlbumSortMode(InsideAlbumSortMode::DiscNumberTrackNumber));
        } else {
            if (mAllAlbums) {
                mSortDescending = Settings::instance()->artistTracksSortDescending();
                mSortMode = static_cast<SortMode>(Settings::instance()->artistTracksSortMode(SortMode::ArtistAlbumYear));
                mInsideAlbumSortMode = static_cast<InsideAlbumSortMode>(Settings::instance()->artistTracksInsideAlbumSortMode(InsideAlbumSortMode::DiscNumberTrackNumber));
            } else {
                mSortDescending = Settings::instance()->albumTracksSortDescending();
                mSortMode = SortMode::ArtistAlbumTitle;
                mInsideAlbumSortMode = static_cast<InsideAlbumSortMode>(Settings::instance()->albumTracksSortMode(InsideAlbumSortMode::DiscNumberTrackNumber));
            }
        }

        emit sortModeChanged();
        emit insideAlbumSortModeChanged();

        execQuery();
    }

    QVariant TracksModel::data(const QModelIndex& index, int role) const
    {
        const Track& track = mTracks[index.row()];

        switch (role) {
        case FilePathRole:
            return track.filePath;
        case TitleRole:
            return track.title;
        case ArtistRole:
            return track.artist;
        case AlbumRole:
            return track.album;
        case DurationRole:
            return track.duration;
        default:
            return QVariant();
        }
    }

    int TracksModel::rowCount(const QModelIndex&) const
    {
        return mTracks.size();
    }

    bool TracksModel::allArtists() const
    {
        return mAllArtists;
    }

    void TracksModel::setAllArtists(bool allArtists)
    {
        mAllArtists = allArtists;
    }

    bool TracksModel::allAlbums() const
    {
        return mAllAlbums;
    }

    void TracksModel::setAllAlbums(bool allAlbums)
    {
        mAllAlbums = allAlbums;
    }

    const QString& TracksModel::artist() const
    {
        return mArtist;
    }

    void TracksModel::setArtist(const QString& artist)
    {
        mArtist = artist;
    }

    const QString& TracksModel::album() const
    {
        return mAlbum;
    }

    void TracksModel::setAlbum(const QString& album)
    {
        mAlbum = album;
    }

    const QString& TracksModel::genre() const
    {
        return mGenre;
    }

    void TracksModel::setGenre(const QString& genre)
    {
        mGenre = genre;
    }

    bool TracksModel::sortDescending() const
    {
        return mSortDescending;
    }

    void TracksModel::setSortDescending(bool descending)
    {
        if (descending != mSortDescending) {
            mSortDescending = descending;
            execQuery();
        }
    }

    TracksModel::SortMode TracksModel::sortMode() const
    {
        return mSortMode;
    }

    void TracksModel::setSortMode(TracksModel::SortMode mode)
    {
        if (mode != mSortMode) {
            mSortMode = mode;
            emit sortModeChanged();
            execQuery();
        }
    }

    TracksModel::InsideAlbumSortMode TracksModel::insideAlbumSortMode() const
    {
        return mInsideAlbumSortMode;
    }

    void TracksModel::setInsideAlbumSortMode(TracksModel::InsideAlbumSortMode mode)
    {
        if (mode != mInsideAlbumSortMode) {
            mInsideAlbumSortMode = mode;
            emit insideAlbumSortModeChanged();
            execQuery();
        }
    }

    bool TracksModel::isRemovingFiles() const
    {
        return mRemovingFiles;
    }

    QStringList TracksModel::getTracks(const std::vector<int>& indexes)
    {
        QStringList tracks;
        tracks.reserve(indexes.size());
        for (int index : indexes) {
            tracks.push_back(mTracks[index].filePath);
        }
        return tracks;
    }

    void TracksModel::removeTrack(int index, bool deleteFile)
    {   
        removeTracks({index}, deleteFile);
    }

    void TracksModel::removeTracks(std::vector<int> indexes, bool deleteFiles)
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
                    QString filePath(mTracks[index].filePath);

                    QSqlQuery query(db);
                    query.prepare(QStringLiteral("DELETE FROM tracks WHERE filePath = ?"));
                    query.addBindValue(filePath);
                    if (query.exec()) {
                        removed.push_back(index);
                    } else {
                        qWarning() << "failed to remove file from database" << query.lastQuery()  << query.lastError();
                    }

                    if (deleteFiles) {
                        if (!QFile::remove(filePath)) {
                            qWarning() << "failed to remove file:" << filePath << query.lastError();
                        }
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
                mTracks.erase(mTracks.begin() + index);
                endRemoveRows();
            }
            emit LibraryUtils::instance()->databaseChanged();
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }

    QHash<int, QByteArray> TracksModel::roleNames() const
    {
        return {{FilePathRole, "filePath"},
                {TitleRole, "title"},
                {ArtistRole, "artist"},
                {AlbumRole, "album"},
            {DurationRole, "duration"}};
    }

    void TracksModel::execQuery()
    {
        QString queryString(QLatin1String("SELECT filePath, title, artist, album, duration FROM tracks "));

        if (mAllArtists) {
            if (!mGenre.isEmpty()) {
                queryString += QLatin1String("WHERE genre = ? ");
            }
            queryString += QLatin1String("GROUP BY id, artist, album ");
        } else {
            queryString += QLatin1String("WHERE artist = ? ");
            if (mAllAlbums) {
                queryString += QLatin1String("GROUP BY id, album ");
            } else {
                queryString += QLatin1String("AND album = ? "
                                       "GROUP BY id ");
            }
        }

        switch (mSortMode) {
        case SortMode::Title:
            queryString += QLatin1String("ORDER BY title %1");
            break;
        case SortMode::AddedDate:
            queryString += QLatin1String("ORDER BY id %1");
            break;
        case SortMode::ArtistAlbumTitle:
            queryString += QLatin1String("ORDER BY artist = '' %1, artist %1, album = '' %1, album %1, ");
            break;
        case SortMode::ArtistAlbumYear:
            queryString += QLatin1String("ORDER BY artist = '' %1, artist %1, album = '' %1, year %1, album %1, ");
            break;
        }

        if (mSortMode == SortMode::ArtistAlbumTitle ||
                mSortMode == SortMode::ArtistAlbumYear) {
            switch (mInsideAlbumSortMode) {
            case InsideAlbumSortMode::Title:
                queryString += QLatin1String("title %1");
                break;
            case InsideAlbumSortMode::DiscNumberTitle:
                queryString += QLatin1String("discNumber = '' %1, discNumber %1, title %1");
                break;
            case InsideAlbumSortMode::DiscNumberTrackNumber:
                queryString += QLatin1String("discNumber = '' %1, discNumber %1, trackNumber %1, title %1");
                break;
            }
        }

        queryString = queryString.arg(mSortDescending ? QLatin1String("DESC")
                                                      : QLatin1String("ASC"));

        QSqlQuery query;
        query.prepare(queryString);

        if (mAllArtists) {
            if (!mGenre.isEmpty()) {
                query.addBindValue(mGenre);
            }
        } else {
            query.addBindValue(mArtist);
            if (!mAllAlbums) {
                query.addBindValue(mAlbum);
            }
        }

        beginResetModel();
        mTracks.clear();
        if (query.exec()) {
            while (query.next()) {
                const QString artist(query.value(ArtistField).toString());
                const QString album(query.value(AlbumField).toString());
                mTracks.push_back({query.value(FilePathField).toString(),
                                   query.value(TitleField).toString(),
                                   artist.isEmpty() ? qApp->translate("unplayer", "Unknown artist") : artist,
                                   album.isEmpty() ? qApp->translate("unplayer", "Unknown album") : album,
                                   query.value(DurationField).toInt()});
            }
        } else {
            qWarning() << query.lastError();
        }
        endResetModel();
    }
}
