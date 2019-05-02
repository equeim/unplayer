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

#include <QCoreApplication>
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
            inline TagLib::String toTString(const QString& string)
            {
                const auto size = static_cast<std::wstring::size_type>(string.size());
                TagLib::String str(std::wstring(size, 0));
                const ushort* utf16 = string.utf16();
                std::copy(utf16, utf16 + size, str.begin());
                return str;
            }

            const char* errorCauseTagExtractionNotSupported = "tag extraction for this file format is not supported";
            const char* errorCauseTagSavingNotSupported = "tag saving for this file format is not supported";
            const char* errorCauseExtensionDoesntMatch = "file format doesn't match extension";
            const char* errorCauseFormatNotSupported = "file format is not supported";

            struct ExtractProcessor
            {
                inline void setMediaArt(const TagLib::ByteVector& data) const
                {
                    info.mediaArtData = QByteArray(data.data(), static_cast<int>(data.size()));
                }

                void getApeMediaArt(const TagLib::APE::Tag* tag) const
                {
                    const TagLib::APE::ItemListMap& items = tag->itemListMap();
                    const auto found(items.find("COVER ART (FRONT)"));
                    if (found != items.end()) {
                        const TagLib::ByteVector data(found->second.binaryData());
                        setMediaArt(data.mid(static_cast<unsigned int>(data.find('\0') + 1)));
                    }
                }

                void setFlacPicture(const TagLib::List<TagLib::FLAC::Picture*>& pictures) const
                {
                    if (!pictures.isEmpty()) {
                        for (const TagLib::FLAC::Picture* picture : pictures) {
                            if (picture->type() == TagLib::FLAC::Picture::FrontCover) {
                                setMediaArt(picture->data());
                                return;
                            }
                        }
                        setMediaArt(pictures.front()->data());
                    }
                }

                bool processFile(TagLib::File&& file) const
                {
                    if (file.isValid()) {
                        info.fileTypeValid = true;
                        info.canReadTags = true;

                        const TagLib::Tag* tag = file.tag();
                        const TagLib::PropertyMap properties(file.properties());

                        info.title = tag->title().toCString(true);
                        info.year = static_cast<int>(tag->year());
                        info.trackNumber = static_cast<int>(tag->track());

                        const TagLib::StringList& artists = properties[ArtistsTag.data()];
                        info.artists.reserve(static_cast<int>(artists.size()));
                        for (const TagLib::String& artist : artists) {
                            info.artists.push_back(artist.toCString(true));
                        }
                        info.artists.removeDuplicates();

                        const TagLib::StringList& albums = properties[AlbumsTag.data()];
                        info.albums.reserve(static_cast<int>(albums.size()));
                        for (const TagLib::String& album : albums) {
                            info.albums.push_back(album.toCString(true));
                        }
                        info.albums.removeDuplicates();

                        const TagLib::StringList& genres = properties[GenresTag.data()];
                        info.genres.reserve(static_cast<int>(genres.size()));
                        for (const TagLib::String& genre : genres) {
                            info.genres.push_back(genre.toCString(true));
                        }
                        info.genres.removeDuplicates();

                        {
                            const auto found = properties.find(DiscNumberTag.data());
                            if (found != properties.end()) {
                                const TagLib::StringList& list = found->second;
                                if (!list.isEmpty()) {
                                    info.discNumber = list[0].toCString(true);
                                }

                            }
                        }

                        const TagLib::AudioProperties* audioProperties = file.audioProperties();
                        info.duration = audioProperties->length();
                        info.bitrate = audioProperties->bitrate();
                    } else {
                        error(errorCauseExtensionDoesntMatch);
                    }
                    return info.fileTypeValid;
                }

                void processFile(TagLib::FLAC::File&& file) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file))) {
                        setFlacPicture(file.pictureList());
                    }
                }

                void processFile(TagLib::MP4::File&& file) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file))) {
                        const TagLib::MP4::ItemMap& items = file.tag()->itemMap();
                        const auto found(items.find("covr"));
                        if (found != items.end()) {
                            const TagLib::MP4::CoverArtList covers(found->second.toCoverArtList());
                            if (!covers.isEmpty()) {
                                setMediaArt(covers.front().data());
                            }
                        }
                    }
                }

                void processFile(TagLib::MPEG::File&& file) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file))) {
                        if (file.hasAPETag()) {
                            getApeMediaArt(file.APETag());
                        }
                        if (info.mediaArtData.isEmpty() && file.hasID3v2Tag()) {
                            const auto set = [&](const TagLib::ID3v2::FrameList& frames) {
                                if (frames.isEmpty()) {
                                    return false;
                                }

                                for (const TagLib::ID3v2::Frame* frame : frames) {
                                    const auto pictureFrame = static_cast<const TagLib::ID3v2::AttachedPictureFrame*>(frame);
                                    if (pictureFrame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
                                        setMediaArt(pictureFrame->picture());
                                        return true;
                                    }
                                }
                                setMediaArt(static_cast<const TagLib::ID3v2::AttachedPictureFrame*>(frames.front())->picture());
                                return true;
                            };

                            const TagLib::ID3v2::FrameListMap& frameListMap = file.ID3v2Tag()->frameListMap();
                            const auto apicFramesFound(frameListMap.find("APIC"));
                            if (apicFramesFound != frameListMap.end()) {
                                if (set(apicFramesFound->second)) {
                                    return;
                                }
                            }
                            const auto picFramesFound(frameListMap.find("PIC"));
                            if (picFramesFound != frameListMap.end()) {
                                set(picFramesFound->second);
                            }
                        }
                    }
                }

                void processFile(TagLib::Ogg::Vorbis::File&& file) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file))) {
                        setFlacPicture(file.tag()->pictureList());
                    }
                }

                void processFile(TagLib::Ogg::Opus::File&& file) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file))) {
                        setFlacPicture(file.tag()->pictureList());
                    }
                }

                void processFile(TagLib::Ogg::FLAC::File&& file) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file))) {
                        setFlacPicture(file.tag()->pictureList());
                    }
                }

                void processFile(TagLib::APE::File&& file) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file)) && file.hasAPETag()) {
                        getApeMediaArt(file.APETag());
                    }
                }

                void processFile(TagLib::WavPack::File&& file) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file)) && file.hasAPETag()) {
                        getApeMediaArt(file.APETag());
                    }
                }

                void checkMimeType(const QLatin1String& mimeType) const
                {
                    if (mimeDb.mimeTypeForFile(info.filePath, QMimeDatabase::MatchContent).name() == mimeType) {
                        error(errorCauseTagExtractionNotSupported);
                        info.fileTypeValid = true;
                    } else {
                        error(errorCauseExtensionDoesntMatch);
                    }
                }

                void error(const char* errorCause) const
                {
                    qWarning().nospace() << "Warning: can't read tags from " << info.filePath << ": " << errorCause;
                }

                Info& info;
                const QMimeDatabase& mimeDb;
            };

            struct SaveProcessor
            {
                void processFile(TagLib::File&& file) const
                {
                    if (file.isValid()) {
                        TagLib::PropertyMap properties(file.properties());
                        for (const auto& i : replaceProperties) {
                            properties.replace(i.first, i.second);
                        }
                        file.setProperties(properties);

                        if (file.save()) {
                            Info info;
                            info.filePath = filePath;
                            ExtractProcessor{info, mimeDb}.processFile(std::move(file));
                            infos.push_back(std::move(info));
                        } else {
                            error("error when saving file");
                        }
                    } else {
                        error(errorCauseExtensionDoesntMatch);
                    }
                }

                void checkMimeType(const QLatin1String&) const
                {
                    error(errorCauseTagSavingNotSupported);
                }

                void error(const char* errorCause) const
                {
                    qWarning().nospace() << "Warning: can't save tags for file " << filePath << ": " << errorCause;
                }

                const QString& filePath;
                const TagLib::PropertyMap& replaceProperties;
                std::vector<Info>& infos;
                const QMimeDatabase& mimeDb;
            };

            const QLatin1String aacMimeType("audio/aac");
            const QLatin1String oggMimeType("application/ogg");
            const QLatin1String oggVorbisMimeType("audio/x-vorbis+ogg");
            const QLatin1String oggOpusMimeType("audio/x-opus+ogg");
            const QLatin1String oggFlacMimeType("audio/x-flac+ogg");
            const QLatin1String matroskaMimeType("application/x-matroska");

            template<class Processor>
            void processFile(const QString& filePath, Extension extension, const QMimeDatabase& mimeDb, const Processor& processor)
            {
                switch (extension) {
                case Extension::FLAC:
                    processor.processFile(TagLib::FLAC::File(filePath.toUtf8()));
                    break;
                case Extension::AAC:
                    processor.checkMimeType(aacMimeType);
                    break;
                case Extension::M4A:
                    processor.processFile(TagLib::MP4::File(filePath.toUtf8()));
                    break;
                case Extension::MP3:
                    processor.processFile(TagLib::MPEG::File(filePath.toUtf8()));
                    break;
                case Extension::OGA:
                case Extension::OGG:
                {
                    const QMimeType mimeType(mimeDb.mimeTypeForFile(filePath, QMimeDatabase::MatchContent));
                    const QString mimeTypeName(mimeType.name());
                    if (mimeTypeName == oggVorbisMimeType) {
                        processor.processFile(TagLib::Ogg::Vorbis::File(filePath.toUtf8()));
                    } else if (mimeTypeName == oggOpusMimeType) {
                        processor.processFile(TagLib::Ogg::Opus::File(filePath.toUtf8()));
                    } else if (mimeTypeName == oggFlacMimeType) {
                        processor.processFile(TagLib::Ogg::FLAC::File(filePath.toUtf8()));
                    } else if (mimeType.inherits(oggMimeType)) {
                        processor.error(errorCauseFormatNotSupported);
                    } else {
                        processor.error(errorCauseExtensionDoesntMatch);
                    }
                    break;
                }
                case Extension::OPUS:
                    processor.processFile(TagLib::Ogg::Opus::File(filePath.toUtf8()));
                    break;
                case Extension::APE:
                    processor.processFile(TagLib::APE::File(filePath.toUtf8()));
                    break;
                case Extension::MKA:
                    processor.checkMimeType(matroskaMimeType);
                    break;
                case Extension::WAV:
                    processor.processFile(TagLib::RIFF::WAV::File(filePath.toUtf8()));
                    break;
                case Extension::WAVPACK:
                    processor.processFile(TagLib::WavPack::File(filePath.toUtf8()));
                    break;
                case Extension::Other:
                    processor.error(errorCauseFormatNotSupported);
                    break;
                }
            }
        }

        const QLatin1String TitleTag("TITLE");
        const QLatin1String ArtistsTag("ARTIST");
        const QLatin1String AlbumsTag("ALBUM");
        const QLatin1String YearTag("DATE");
        const QLatin1String TrackNumberTag("TRACKNUMBER");
        const QLatin1String GenresTag("GENRE");
        const QLatin1String DiscNumberTag("DISCNUMBER");

        Info getTrackInfo(const QFileInfo& fileInfo, const QMimeDatabase& mimeDb)
        {
            Info info;
            const QString filePath(fileInfo.filePath());
            info.filePath = filePath;
            processFile(filePath, extensionFromSuffux(fileInfo.suffix()), mimeDb, ExtractProcessor{info, mimeDb});
            if (info.fileTypeValid && info.title.isEmpty()) {
                info.title = fileInfo.fileName();
            }
            return info;
        }

        template<bool IncrementTrackNumber>
        std::vector<Info> saveTags(const QStringList& files, const QVariantMap& tags, const QMimeDatabase& mimeDb)
        {
            std::vector<Info> infos;
            infos.reserve(static_cast<std::vector<Info>::size_type>(files.size()));

            TagLib::PropertyMap replaceProperties;
            for (auto i = tags.begin(), end = tags.end(); i != end; ++i) {
                TagLib::StringList values;
                for (const QString& value : i.value().toStringList()) {
                    values.append(toTString(value));
                }
                replaceProperties.insert(toTString(i.key()), std::move(values));
            }

            int trackNumber;
            TagLib::String* trackNumberString;
            if (IncrementTrackNumber) {
                trackNumberString = &(replaceProperties[TrackNumberTag.data()][0]);
                trackNumber = trackNumberString->toInt();
            }

            for (const QString& filePath : files) {
                if (!qApp) {
                    break;
                }
                processFile(filePath, extensionFromSuffux(QFileInfo(filePath).suffix()), mimeDb, SaveProcessor{filePath, replaceProperties, infos, mimeDb});
                if (IncrementTrackNumber) {
                    ++trackNumber;
                    *trackNumberString = TagLib::String::number(trackNumber);
                }
            }

            return infos;
        }

        template std::vector<Info> saveTags<true>(const QStringList& files, const QVariantMap& tags, const QMimeDatabase& mimeDb);
        template std::vector<Info> saveTags<false>(const QStringList& files, const QVariantMap& tags, const QMimeDatabase& mimeDb);
    }
}
