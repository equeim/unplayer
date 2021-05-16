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

#ifndef UNPLAYER_TAGUTILS_H
#define UNPLAYER_TAGUTILS_H

#include <functional>
#include <optional>
#include <vector>

#include <QObject>
#include <QString>
#include <QStringList>

#include "fileutils.h"

template<typename K, typename V> class QMap;
class QMimeDatabase;
class QVariant;
typedef QMap<QString, QVariant> QVariantMap;

namespace unplayer
{
    class Tags final : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString title READ title CONSTANT)
        Q_PROPERTY(QString artists READ artists CONSTANT)
        Q_PROPERTY(QString albumArtists READ albumArtists CONSTANT)
        Q_PROPERTY(QString albums READ albums CONSTANT)
        Q_PROPERTY(QString year READ year CONSTANT)
        Q_PROPERTY(QString trackNumber READ trackNumber CONSTANT)
        Q_PROPERTY(QString genres READ genres CONSTANT)
        Q_PROPERTY(QString discNumber READ discNumber CONSTANT)
    public:
        static QLatin1String title();
        static QLatin1String artists();
        static QLatin1String albumArtists();
        static QLatin1String albums();
        static QLatin1String year();
        static QLatin1String trackNumber();
        static QLatin1String genres();
        static QLatin1String discNumber();
    };

    namespace tagutils
    {
        struct Info
        {
            inline Info() = default;
            inline Info(const QString& filePath, fileutils::AudioCodec audioCodec, bool canReadTags)
                : filePath(filePath), audioCodec(audioCodec), canReadTags(canReadTags) {}

            QString filePath;
            fileutils::AudioCodec audioCodec = fileutils::AudioCodec::Unknown;
            bool canReadTags{};

            QString title;
            QStringList artists;
            QStringList albumArtists;
            QStringList albums;
            int year{};
            int trackNumber{};
            QStringList genres;
            QString discNumber;

            int duration{};
            int bitrate{};
            int bitDepth{};
            int sampleRate{};
            int channels{};

            QByteArray mediaArtData;
        };

        struct AudioCodecInfo
        {
            inline AudioCodecInfo() = default;
            inline explicit AudioCodecInfo(fileutils::AudioCodec audioCodec) : audioCodec(audioCodec) {}
            fileutils::AudioCodec audioCodec = fileutils::AudioCodec::Unknown;
            int bitDepth{};
            int sampleRate{};
            int bitrate{};
        };

        std::optional<Info> getTrackInfo(const QString& filePath, fileutils::Extension extension);
        std::optional<QByteArray> getTackMediaArtData(const QString& filePath, fileutils::Extension extension);
        std::optional<AudioCodecInfo> getTrackAudioCodecInfo(const QString& filePath, fileutils::Extension extension);

        template<bool IncrementTrackNumber>
        std::vector<Info> saveTags(const QStringList& files, const QVariantMap& tags, const std::function<void(Info&)>& callback);

        extern template std::vector<Info> saveTags<true>(const QStringList& files, const QVariantMap& tags, const std::function<void(Info&)>& callback);
        extern template std::vector<Info> saveTags<false>(const QStringList& files, const QVariantMap& tags, const std::function<void(Info&)>& callback);
    }
}

#endif // UNPLAYER_TAGUTILS_H
