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

#ifndef UNPLAYER_PLAYLISTMODEL_H
#define UNPLAYER_PLAYLISTMODEL_H

#include <QAbstractListModel>
#include <QQmlParserStatus>

class QSparqlResult;

namespace unplayer
{
    struct PlaylistTrack;

    class PlaylistModel : public QAbstractListModel, public QQmlParserStatus
    {
        Q_OBJECT
        Q_INTERFACES(QQmlParserStatus)
        Q_PROPERTY(QString url READ url WRITE setUrl)
        Q_PROPERTY(bool loaded READ isLoaded NOTIFY loadedChanged)
    public:
        PlaylistModel();
        ~PlaylistModel() override;
        void classBegin() override;
        void componentComplete() override;

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;

        QString url() const;
        void setUrl(const QString& newUrl);

        bool isLoaded() const;

        Q_INVOKABLE QVariantMap get(int trackIndex) const;
        Q_INVOKABLE void removeAtIndexes(const QList<int>& trackIndexes);

    protected:
        QHash<int, QByteArray> roleNames() const override;

    private:
        void onQueryFinished();

    private:
        QList<PlaylistTrack*> mTracks;
        int mRowCount;

        QList<QSparqlResult*> mQueries;
        int mLoadedTracks;

        QString mUrl;
        bool mLoaded;
    signals:
        void loadedChanged();
    };
}

#endif // UNPLAYER_PLAYLISTMODEL_H
