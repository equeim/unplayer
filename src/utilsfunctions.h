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

#ifndef UNPLAYER_UTILSFUNCTIONS_H
#define UNPLAYER_UTILSFUNCTIONS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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
    inline void forMaxCountInRange(int max, int maxCount, Function call)
    {
        for (int i = 0; i < max; i += maxCount) {
            const int count = [&]() {
                const int left = max - i;
                if (left > maxCount) {
                    return maxCount;
                }
                return left;
            }();
            call(i, count);
        }
    }
}

#endif // UNPLAYER_UTILSFUNCTIONS_H
