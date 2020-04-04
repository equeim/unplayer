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

#ifndef UNPLAYER_STDUTILS_H
#define UNPLAYER_STDUTILS_H

#include <algorithm>
#include <memory>

#include <iterator>
#include <type_traits>

#include <QtGlobal>
#include <QHashFunctions>
#include <QUrl>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
/*
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
*/
#define QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH(Class, Arguments)      \
    QT_BEGIN_INCLUDE_NAMESPACE                                      \
    namespace std {                                                 \
        template <>                                                 \
        struct hash< QT_PREPEND_NAMESPACE(Class) > {                \
            using argument_type = QT_PREPEND_NAMESPACE(Class);      \
            using result_type = size_t;                             \
            size_t operator()(Arguments s) const                    \
                noexcept(noexcept(QT_PREPEND_NAMESPACE(qHash)(s)))  \
            {                                                       \
                /* this seeds qHash with the result of */           \
                /* std::hash applied to an int, to reap */          \
                /* any protection against predictable hash */       \
                /* values the std implementation may provide */     \
                return QT_PREPEND_NAMESPACE(qHash)(s,               \
                           QT_PREPEND_NAMESPACE(qHash)(             \
                                      std::hash<int>{}(0)));        \
            }                                                       \
        };                                                          \
    }                                                               \
    QT_END_INCLUDE_NAMESPACE                                        \
    /*end*/

#define QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF(Class) \
    QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH(Class, const argument_type &)

QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF(QString)
QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF(QByteArray)
QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF(QUrl)
#endif

namespace unplayer
{
    template<typename C, typename V>
    inline auto contains_impl(const C& container, const V& value, int) -> decltype(container.find(value), true)
    {
        return container.find(value) != std::end(container);
    }

    template<typename C, typename V>
    inline bool contains_impl(const C& container, const V& value, long)
    {
        return std::find(std::begin(container), std::end(container), value) != std::end(container);
    }

    template<typename C, typename V>
    inline bool contains(const C& container, const V& value)
    {
        return contains_impl(container, value, 0);
    }

    template<typename C, typename V>
    inline size_t index_of(const C& container, const V& value) {
        return static_cast<size_t>(std::find(std::begin(container), std::end(container), value) - std::begin(container));
    }

    template<typename C, typename V>
    inline int index_of_i(const C& container, const V& value) {
        return static_cast<int>(std::find(std::begin(container), std::end(container), value) - std::begin(container));
    }

    template<typename C, typename V>
    inline void erase_one(C& container, const V& value) {
        container.erase(std::find(container.begin(), container.end(), value));
    }

    template<typename T>
    inline bool operator==(const std::shared_ptr<T>& shared, const T* raw) {
        return shared.get() == raw;
    }
}

#endif // UNPLAYER_STDUTILS_H
