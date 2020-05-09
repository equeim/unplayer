/*
 * Unplayer
 * Copyright (C) 2015-2020 Alexey Rochev <equeim@gmail.com>
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

#ifndef UNPLAYER_ASYNCLOADINGMODEL_H
#define UNPLAYER_ASYNCLOADINGMODEL_H

#include <QAbstractListModel>

namespace unplayer
{
    class AsyncLoadingModel : public QAbstractListModel
    {
        Q_OBJECT
        Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    public:
        inline bool isLoading() { return mLoading; }

    protected:
        inline void setLoading(bool loading)
        {
            if (loading != mLoading) {
                mLoading = loading;
                emit loadingChanged(loading);
                if (!loading) {
                    emit loaded();
                }
            }
        }

    private:
        bool mLoading = true;

    signals:
        void loadingChanged(bool loading);
        void loaded();
    };
}

#endif // UNPLAYER_ASYNCLOADINGMODEL_H
