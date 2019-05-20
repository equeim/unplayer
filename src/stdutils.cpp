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

#include "stdutils.h"

#include <QHash>
#include <QUrl>

namespace std
{
    size_t hash<QString>::operator()(const QString& string) const
    {
        return qHash(string);
    }

    size_t hash<QByteArray>::operator()(const QByteArray& string) const
    {
        return qHash(string);
    }

    size_t hash<QUrl>::operator()(const QUrl& string) const
    {
        return qHash(string);
    }
}
