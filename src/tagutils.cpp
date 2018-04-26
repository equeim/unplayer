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
#include <xiphcomment.h>

namespace unplayer
{
    namespace tagutils
    {
        namespace
        {
            enum class VorbisComment
            {
                Artist,
                Album,
                Genre
            };

            void getTags(const TagLib::Tag* tag, const TagLib::PropertyMap& properties, Info& info)
            {
                info.title = tag->title().toCString(true);
                info.year = tag->year();
                info.trackNumber = tag->track();

                for (const TagLib::String& artist : properties["ARTIST"]) {
                    info.artists.append(artist.toCString(true));
                }

                for (const TagLib::String& album : properties["ALBUM"]) {
                    info.albums.append(album.toCString(true));
                }

                for (const TagLib::String& genre : properties["GENRE"]) {
                    info.genres.append(genre.toCString(true));
                }
            }

            void getAudioProperties(const TagLib::File& file, Info& info)
            {
                const TagLib::AudioProperties* audioProperties = file.audioProperties();
                if (audioProperties) {
                    info.duration = audioProperties->length();
                    info.bitrate = audioProperties->bitrate();
                }
            }

            void setMediaArt(const TagLib::ByteVector& data, Info& info)
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
        }

        Info getTrackInfo(const QFileInfo& fileInfo, const QString& mimeType)
        {
            Info info;

            switch (mimeTypeFromString(mimeType)) {
            case MimeType::Flac:
            {
                TagLib::FLAC::File file(fileInfo.filePath().toUtf8().data());
                getAudioProperties(file, info);
                if (file.hasID3v2Tag()) {
                    getTags(file.ID3v2Tag(), file.ID3v2Tag()->properties(), info);
                } else if (file.hasXiphComment()) {
                    getTags(file.xiphComment(), file.xiphComment()->properties(), info);
                }
                getFlacMediaArt(file, info);
                break;
            }
            case MimeType::Mp4:
            case MimeType::Mp4b:
            {
                const TagLib::MP4::File file(fileInfo.filePath().toUtf8().data());
                getAudioProperties(file, info);
                if (file.hasMP4Tag()) {
                    getTags(file.tag(), file.tag()->properties(), info);
                    getMp4MediaArt(file.tag(), info);
                }
                break;
            }
            case MimeType::Mpeg:
            {
                TagLib::MPEG::File file(fileInfo.filePath().toUtf8().data());
                getAudioProperties(file, info);
                if (file.hasAPETag()) {
                    getTags(file.APETag(), file.APETag()->properties(), info);
                    getApeMediaArt(file.APETag(), info);
                } else if (file.hasID3v2Tag()) {
                    getId3v2MediaArt(file.ID3v2Tag(), info);
                    getTags(file.ID3v2Tag(), file.ID3v2Tag()->properties(), info);
                }
                break;
            }
            case MimeType::VorbisOgg:
            {
                const TagLib::Ogg::Vorbis::File file(fileInfo.filePath().toUtf8().data());
                getAudioProperties(file, info);
                getTags(file.tag(), file.tag()->properties(), info);
                getXiphMediaArt(file.tag(), info);
                break;
            }
            case MimeType::FlacOgg:
            {
                const TagLib::Ogg::FLAC::File file(fileInfo.filePath().toUtf8().data());
                getAudioProperties(file, info);
                getTags(file.tag(), file.tag()->properties(), info);
                getXiphMediaArt(file.tag(), info);
                break;
            }
            case MimeType::OpusOgg:
            {
                const TagLib::Ogg::Opus::File file(fileInfo.filePath().toUtf8().data());
                getAudioProperties(file, info);
                getTags(file.tag(), file.tag()->properties(), info);
                getXiphMediaArt(file.tag(), info);
                break;
            }
            case MimeType::Ape:
            {
                TagLib::APE::File file(fileInfo.filePath().toUtf8().data());
                getAudioProperties(file, info);
                if (file.hasAPETag()) {
                    getApeMediaArt(file.APETag(), info);
                    getTags(file.APETag(), file.APETag()->properties(), info);
                }
                break;
            }
            default:
            {
                const TagLib::FileRef file(fileInfo.filePath().toUtf8().data());
                if (file.file()) {
                    getAudioProperties(*file.file(), info);
                    if (file.tag()) {
                        getTags(file.tag(), file.tag()->properties(), info);
                    }
                }
            }
            }

            if (info.title.isEmpty()) {
                info.title = fileInfo.fileName();
            }

            return info;
        }
    }
}
