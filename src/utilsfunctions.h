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

#ifndef UNPLAYER_UTILSFUNCTIONS_H
#define UNPLAYER_UTILSFUNCTIONS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <QFuture>
#include <QFutureWatcher>
#include <QString>

namespace unplayer
{
    inline long long getLastModifiedTime(const QString& filePath)
    {
        struct stat64 result;
        if (stat64(filePath.toUtf8(), &result) != 0) {
            return -1;
        }
        return result.st_mtim.tv_sec * 1000 + result.st_mtim.tv_nsec / 1000000;
    }

    template<typename Function>
    inline void batchedCount(size_t count, size_t batchSize, Function call)
    {
        for (size_t i = 0; i < count; i += batchSize) {
            const size_t nextCount = [&]() {
                const size_t left = count - i;
                if (left > batchSize) {
                    return batchSize;
                }
                return left;
            }();
            call(i, nextCount);
        }
    }

    template<typename T, typename Func>
    inline void onFutureFinished(const QFuture<T>& future, QObject* parent, const Func& func)
    {
        auto watcher = new QFutureWatcher<T>(parent);
        QObject::connect(watcher, &QFutureWatcher<T>::finished, parent, [watcher, func]() {
            func(watcher->result());
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }

    template<typename Func>
    inline void onFutureFinished(const QFuture<void>& future, QObject* parent, const Func& func)
    {
        auto watcher = new QFutureWatcher<void>(parent);
        QObject::connect(watcher, &QFutureWatcher<void>::finished, parent, [watcher, func]() {
            func();
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }

    inline QString unquote(QString str) {
        static const QLatin1Char quoteChar('"');
        if (str.startsWith(quoteChar) && str.endsWith(quoteChar) && str.size() > 1) {
            str.chop(1);
            str.remove(0, 1);
        }
        return str;
    }
}

#endif // UNPLAYER_UTILSFUNCTIONS_H
