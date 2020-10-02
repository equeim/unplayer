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

#include "fileutils.h"

#include <unordered_map>
#include <unordered_set>

#include <QLatin1String>

#include "stdutils.h"

namespace unplayer
{
    namespace fileutils
    {
        namespace
        {
            const QLatin1String flacSuffix("flac");
            const QLatin1String aacSuffix("aac");

            const QLatin1String m4aSuffix("m4a");
            const QLatin1String f4aSuffix("f4a");
            const QLatin1String m4bSuffix("m4b");
            const QLatin1String f4bSuffix("f4b");

            const QLatin1String mp3Suffix("mp3");
            const QLatin1String mpgaSuffix("mpga");

            const QLatin1String ogaSuffix("oga");
            const QLatin1String oggSuffix("ogg");
            const QLatin1String opusSuffix("opus");

            const QLatin1String apeSuffix("ape");

            const QLatin1String mkaSuffix("mka");

            const QLatin1String wavSuffix("wav");

            //const QLatin1String wvSuffix("wv");
            //const QLatin1String wvpSuffix("wvp");
        }

        Extension extensionFromSuffix(const QString& suffix)
        {
            return extensionFromSuffixLowered(suffix.toLower());
        }

        Extension extensionFromSuffixLowered(const QString& suffixLowered)
        {
            static const std::unordered_map<QString, Extension> extensions{
                {flacSuffix, Extension::FLAC},
                {aacSuffix, Extension::AAC},

                {m4aSuffix, Extension::M4A},
                {f4aSuffix, Extension::M4A},
                {m4bSuffix, Extension::M4A},
                {f4bSuffix, Extension::M4A},

                {mp3Suffix, Extension::MP3},
                {mpgaSuffix, Extension::MP3},

                {ogaSuffix, Extension::OGG},
                {oggSuffix, Extension::OGG},
                {opusSuffix, Extension::OPUS},

                {apeSuffix, Extension::APE},

                {mkaSuffix, Extension::MKA},

                {wavSuffix, Extension::WAV},

                //{wvSuffix, Extension::WAVPACK},
                //{wvpSuffix, Extension::WAVPACK}
            };
            static const auto end(extensions.end());

            const auto found(extensions.find(suffixLowered));
            if (found == end) {
                return Extension::Other;
            }
            return found->second;
        }

        bool isExtensionSupported(const QString& suffix)
        {
            return extensionFromSuffix(suffix) != Extension::Other;
        }

        bool isVideoExtensionSupported(const QString& suffix)
        {
            static const std::unordered_set<QString> videoMimeTypesExtensions{
                QLatin1String("mp4"),
                QLatin1String("m4v"),
                QLatin1String("f4v"),
                QLatin1String("lrv"),

                /*QLatin1String("mpeg"),
                QLatin1String("mpg"),
                QLatin1String("mp2"),
                QLatin1String("mpe"),
                QLatin1String("vob"),*/

                QLatin1String("mkv"),

                QLatin1String("ogv")
            };
            return contains(videoMimeTypesExtensions, suffix.toLower());
        }

        bool isAudioCodecSupported(AudioCodec audioCodec)
        {
            switch (audioCodec) {
            case AudioCodec::ALAC:
            case AudioCodec::Unknown:
                return false;
            default:
                return true;
            }
        }

        QString audioCodecDisplayName(AudioCodec fileType)
        {
            switch (fileType) {
            case AudioCodec::FLAC:
                return QLatin1String("FLAC");
            case AudioCodec::AAC:
                return QLatin1String("AAC");
            case AudioCodec::ALAC:
                return QLatin1String("ALAC");
            case AudioCodec::MP3:
                return QLatin1String("MP3");
            case AudioCodec::Vorbis:
                return QLatin1String("Vorbis");
            case AudioCodec::Opus:
                return QLatin1String("Opus");
            case AudioCodec::APE:
                return QLatin1String("APE");
            case AudioCodec::WAVPACK:
                return QLatin1String("WavPack");
            case AudioCodec::LPCM:
                return QLatin1String("LPCM");
            case AudioCodec::Unknown:
            default:
                return QString();
            }
        }
    }
}
