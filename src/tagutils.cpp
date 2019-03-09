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

#include "tagutils.h"

#include <QDebug>
#include <QFileInfo>

#include <apefile.h>
#include <apetag.h>
#include <attachedpictureframe.h>
#include <fileref.h>
#include <flacfile.h>
#include <id3v2tag.h>
#include <mp4file.h>
#include <mpegfile.h>
#include <oggflacfile.h>
#include <opusfile.h>
#include <tpropertymap.h>
#include <vorbisfile.h>
#include <wavfile.h>
#include <wavpackfile.h>
#include <xiphcomment.h>

namespace unplayer
{
    namespace tagutils
    {
        namespace
        {
            inline void setMediaArt(const TagLib::ByteVector& data, Info& info)
            {
                info.mediaArtData = QByteArray(data.data(), data.size());
            }

            void getFlacMediaArt(TagLib::FLAC::File& file, Info& info)
            {
                const TagLib::List<TagLib::FLAC::Picture*> pictures(file.pictureList());
                if (!pictures.isEmpty()) {
                    setMediaArt(pictures.front()->data(), info);
                }
            }

            void getApeMediaArt(const TagLib::APE::Tag* tag, Info& info)
            {
                if (!tag) {
                    return;
                }

                const TagLib::APE::ItemListMap items(tag->itemListMap());

                if (items.contains("COVER ART (FRONT)")) {
                    const TagLib::APE::Item item(items["COVER ART (FRONT)"]);
                    TagLib::ByteVector data(item.binaryData());
                    data = data.mid(data.find('\0') + 1);
                    setMediaArt(data, info);
                }
            }

            void getMp4MediaArt(const TagLib::MP4::Tag* tag, Info& info)
            {
                if (!tag) {
                    return;
                }

                const TagLib::MP4::Item coverItem(tag->item("covr"));
                if (coverItem.isValid()) {
                    const TagLib::MP4::CoverArtList covers(coverItem.toCoverArtList());
                    if (!covers.isEmpty()) {
                        setMediaArt(covers.front().data(), info);
                    }
                }
            }

            void getId3v2MediaArt(const TagLib::ID3v2::Tag* tag, Info& info)
            {
                if (!tag) {
                    return;
                }

                const TagLib::ID3v2::FrameList picFrames(tag->frameList("PIC"));
                if (picFrames.isEmpty()) {
                    const TagLib::ID3v2::FrameList apicFrames(tag->frameList("APIC"));
                    if (!apicFrames.isEmpty()) {
                        setMediaArt(static_cast<const TagLib::ID3v2::AttachedPictureFrame*>(apicFrames.front())->picture(), info);
                    }
                } else {
                    setMediaArt(static_cast<const TagLib::ID3v2::AttachedPictureFrameV22*>(picFrames.front())->picture(), info);
                }
            }

            void getXiphMediaArt(TagLib::Ogg::XiphComment* tag, Info& info)
            {
                if (!tag) {
                    return;
                }

                const TagLib::List<TagLib::FLAC::Picture*> pictures(tag->pictureList());
                if (!pictures.isEmpty()) {
                    setMediaArt(pictures.front()->data(), info);
                }
            }

            inline void printError(const QString& filePath)
            {
                qWarning() << "Warning: can't read tags from" << filePath;
            }

            bool fillFromFile(Info& info, const TagLib::File& file, const QString& filePath)
            {
                if (file.isValid()) {
                    info.isValid = true;

                    const TagLib::Tag* tag = file.tag();
                    const TagLib::PropertyMap properties(tag->properties());

                    info.title = tag->title().toCString(true);
                    info.year = tag->year();
                    info.trackNumber = tag->track();

                    const TagLib::StringList& artists = properties["ARTIST"];
                    info.artists.reserve(artists.size());
                    for (const TagLib::String& artist : artists) {
                        info.artists.push_back(artist.toCString(true));
                    }
                    info.artists.removeDuplicates();

                    const TagLib::StringList& albums = properties["ALBUM"];
                    info.albums.reserve(albums.size());
                    for (const TagLib::String& album : albums) {
                        info.albums.push_back(album.toCString(true));
                    }
                    info.albums.removeDuplicates();

                    const TagLib::StringList& genres = properties["GENRE"];
                    info.genres.reserve(genres.size());
                    for (const TagLib::String& genre : genres) {
                        info.genres.push_back(genre.toCString(true));
                    }
                    info.genres.removeDuplicates();

                    if (properties.contains("DISCNUMBER")) {
                        const TagLib::StringList list(properties["DISCNUMBER"]);
                        if (!list.isEmpty()) {
                            info.discNumber = list[0].toCString(true);
                        }
                    }

                    const TagLib::AudioProperties* audioProperties = file.audioProperties();
                    info.duration = audioProperties->length();
                    info.bitrate = audioProperties->bitrate();
                } else {
                    info.isValid = false;
                    printError(filePath);
                }
                return info.isValid;
            }
        }

        Info getTrackInfo(const QFileInfo& fileInfo, const QMimeDatabase& mimeDb)
        {
            Info info;

            const QString filePath(fileInfo.filePath());
            const QByteArray filePathBytes(filePath.toUtf8());

            switch (extensionFromSuffux(fileInfo.suffix())) {
            case Extension::FLAC:
            {
                TagLib::FLAC::File file(filePathBytes);
                if (fillFromFile(info, file, filePath)) {
                    getFlacMediaArt(file, info);
                }
                break;
            }
            case Extension::AAC:
            {
                if (mimeDb.mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name() != QLatin1String("audio/aac")) {
                    info.isValid = false;
                } else {
                    printError(filePath);
                }
                break;
            }
            case Extension::M4A:
            {
                const TagLib::MP4::File file(filePathBytes);
                if (fillFromFile(info, file, filePath) && file.hasMP4Tag()) {
                    getMp4MediaArt(file.tag(), info);
                }
                break;
            }
            case Extension::MP3:
            {
                TagLib::MPEG::File file(filePathBytes);
                if (fillFromFile(info, file, filePath)) {
                    if (file.hasAPETag()) {
                        getApeMediaArt(file.APETag(), info);
                    }
                    if (info.mediaArtData.isEmpty() && file.hasID3v2Tag()) {
                        getId3v2MediaArt(file.ID3v2Tag(), info);
                    }
                }
                break;
            }
            case Extension::OGA:
            case Extension::OGG:
            {
                const QString mimeType(mimeDb.mimeTypeForFile(fileInfo.filePath(), QMimeDatabase::MatchContent).name());
                if (mimeType == QLatin1String("audio/x-vorbis+ogg")) {
                    const TagLib::Ogg::Vorbis::File file(filePathBytes);
                    if (fillFromFile(info, file, filePath)) {
                        getXiphMediaArt(file.tag(), info);
                    }
                } else if (mimeType == QLatin1String("audio/x-flac+ogg")) {
                    const TagLib::Ogg::FLAC::File file(filePathBytes);
                    if (fillFromFile(info, file, filePath)) {
                        getXiphMediaArt(file.tag(), info);
                    }
                } else if (mimeType == QLatin1String("audio/x-opus+ogg")) {
                    const TagLib::Ogg::Opus::File file(filePathBytes);
                    if (fillFromFile(info, file, filePath)) {
                        getXiphMediaArt(file.tag(), info);
                    }
                } else {
                    printError(filePath);
                }
                break;
            }
            case Extension::OPUS:
            {
                const TagLib::Ogg::Opus::File file(filePathBytes);
                if (fillFromFile(info, file, filePath)) {
                    getXiphMediaArt(file.tag(), info);
                }
                break;
            }
            case Extension::APE:
            {
                TagLib::APE::File file(filePathBytes);
                if (fillFromFile(info, file, filePath) && file.hasAPETag()) {
                    getApeMediaArt(file.APETag(), info);
                }
                break;
            }
            case Extension::MKA:
            {
                if (mimeDb.mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name() != QLatin1String("audio/x-matroska")) {
                    info.isValid = false;
                } else {
                    printError(filePath);
                }
                break;
            }
            case Extension::WAV:
            {
                if (!fillFromFile(info, TagLib::RIFF::WAV::File(filePathBytes), filePath)) {
                    printError(filePath);
                }
                break;
            }
            case Extension::WAVPACK:
            {
                TagLib::WavPack::File file(filePathBytes);
                if (fillFromFile(info, file, filePath) && file.hasAPETag()) {
                    getApeMediaArt(file.APETag(), info);
                }
                break;
            }
            case Extension::Other:
            default:
            {
                printError(filePath);
                break;
            }
            }

            if (info.title.isEmpty()) {
                info.title = fileInfo.fileName();
            }

            return info;
        }
    }
}
