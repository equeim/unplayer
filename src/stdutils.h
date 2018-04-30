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

#ifndef UNPLAYER_STDUTILS_H
#define UNPLAYER_STDUTILS_H

#include <functional>
#include <QHash>
#include <QString>

namespace std {
    template<>
    class hash<QString> {
    public:
        size_t operator()(const QString& string) const
        {
            return qHash(string);
        }
    };

    template<>
    class hash<QByteArray> {
    public:
        size_t operator()(const QByteArray& bytes) const
        {
            return qHash(bytes);
        }
    };
}


namespace unplayer
{
    template<class T>
    inline bool contains(const T& container, const typename T::value_type& value) {
        return std::find(container.cbegin(), container.cend(), value) != container.cend();
    }

    template<class T>
    inline typename T::difference_type index_of(const T& container, const typename T::value_type& value) {
        return std::find(container.cbegin(), container.cend(), value) - container.cbegin();
    }

    template<class T>
    inline void erase_one(T& container, const typename T::value_type& value) {
        container.erase(std::find(container.begin(), container.end(), value));
    }
}

#endif // UNPLAYER_STDUTILS_H
