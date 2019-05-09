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

#ifndef UNPLAYER_TAGUTILS_H
#define UNPLAYER_TAGUTILS_H

#include <QString>
#include <QPixmap>

#include "libraryutils.h"

class QFileInfo;

namespace unplayer
{
    namespace tagutils
    {
        extern const QLatin1String TitleTag;
        extern const QLatin1String ArtistsTag;
        extern const QLatin1String AlbumsTag;
        extern const QLatin1String YearTag;
        extern const QLatin1String TrackNumberTag;
        extern const QLatin1String GenresTag;
        extern const QLatin1String DiscNumberTag;

        struct Info
        {
            QString filePath;
            QString title;
            QStringList artists;
            QStringList albums;
            int year;
            int trackNumber;
            QStringList genres;
            QString discNumber;
            int duration;
            int bitrate;
            QByteArray mediaArtData;
            bool fileTypeValid;
            bool canReadTags;
        };

        Info getTrackInfo(const QFileInfo& fileInfo, Extension extension, const QMimeDatabase& mimeDb);

        template<bool IncrementTrackNumber>
        std::vector<Info> saveTags(const QStringList& files, const QVariantMap& tags, const QMimeDatabase& mimeDb);

        extern template std::vector<Info> saveTags<true>(const QStringList& files, const QVariantMap& tags, const QMimeDatabase& mimeDb);
        extern template std::vector<Info> saveTags<false>(const QStringList& files, const QVariantMap& tags, const QMimeDatabase& mimeDb);
    }
}

#endif // UNPLAYER_TAGUTILS_H
