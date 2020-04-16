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

#ifndef UNPLAYER_FILEUTILS_H
#define UNPLAYER_FILEUTILS_H

#include <QString>

namespace unplayer
{
    namespace fileutils
    {
        enum class Extension
        {
            FLAC,
            AAC,

            M4A,
            MP3,

            OGG,
            OPUS,

            APE,
            MKA,

            WAV,
            WAVPACK,

            Other
        };

        Extension extensionFromSuffix(const QString& suffix);
        bool isExtensionSupported(const QString& suffix);
        bool isVideoExtensionSupported(const QString& suffix);
    }
}

#endif // UNPLAYER_FILEUTILS_H
