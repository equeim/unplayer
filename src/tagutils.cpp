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
#include <QMimeDatabase>

#include <apefile.h>
#include <attachedpictureframe.h>
//#include <ebmlmatroskafile.h>
#include <flacfile.h>
#include <id3v2tag.h>
#include <mp4file.h>
#include <mpegfile.h>
#include <oggflacfile.h>
#include <opusfile.h>
#include <tpicturemap.h>
#include <tpropertymap.h>
#include <vorbisfile.h>
#include <wavfile.h>
#include <wavpackfile.h>

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

            inline QString toQString(const TagLib::String& string)
            {
                QString str(static_cast<int>(string.size()), QChar());
                std::copy(string.begin(), string.end(), str.begin());
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

                bool processFile(TagLib::File&& file) const
                {
                    if (file.isValid()) {
                        info.fileTypeValid = true;
                        info.canReadTags = true;

                        const TagLib::Tag* tag = file.tag();
                        const TagLib::PropertyMap properties(file.properties());

                        info.title = toQString(tag->title());
                        info.year = static_cast<int>(tag->year());
                        info.trackNumber = static_cast<int>(tag->track());

                        const TagLib::StringList& artists = properties[ArtistsTag.data()];
                        info.artists.reserve(static_cast<int>(artists.size()));
                        for (const TagLib::String& artist : artists) {
                            info.artists.push_back(toQString(artist));
                        }
                        info.artists.removeDuplicates();

                        const TagLib::StringList& albumArtists = properties[AlbumArtistsTag.data()];
                        info.albumArtists.reserve(static_cast<int>(albumArtists.size()));
                        for (const TagLib::String& albumArtist : albumArtists) {
                            info.albumArtists.push_back(toQString(albumArtist));
                        }

                        const TagLib::StringList& albums = properties[AlbumsTag.data()];
                        info.albums.reserve(static_cast<int>(albums.size()));
                        for (const TagLib::String& album : albums) {
                            info.albums.push_back(toQString(album));
                        }
                        info.albums.removeDuplicates();

                        const TagLib::StringList& genres = properties[GenresTag.data()];
                        info.genres.reserve(static_cast<int>(genres.size()));
                        for (const TagLib::String& genre : genres) {
                            info.genres.push_back(toQString(genre));
                        }
                        info.genres.removeDuplicates();

                        {
                            const auto found = properties.find(DiscNumberTag.data());
                            if (found != properties.end()) {
                                const TagLib::StringList& list = found->second;
                                if (!list.isEmpty()) {
                                    info.discNumber = toQString(list[0]);
                                }

                            }
                        }

                        const TagLib::PictureMap pictures(tag->pictures());
                        if (!pictures.isEmpty()) {
                            bool foundFrontCover = false;
                            for (const auto& i : pictures) {
                                if (i.first == TagLib::Picture::FrontCover) {
                                    const TagLib::PictureList& list = i.second;
                                    if (!list.isEmpty()) {
                                        setMediaArt(list.front().data());
                                        foundFrontCover = true;
                                    }
                                    break;
                                }
                            }
                            if (!foundFrontCover) {
                                const TagLib::PictureList& list = pictures.begin()->second;
                                if (!list.isEmpty()) {
                                    setMediaArt(list.front().data());
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

                void processFile(TagLib::MPEG::File&& file) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file))) {
                        if (info.mediaArtData.isEmpty() && file.hasID3v2Tag()) {
                            const TagLib::ID3v2::FrameListMap& frameListMap = file.ID3v2Tag()->frameListMap();
                            const auto picFramesFound(frameListMap.find("PIC"));
                            if (picFramesFound != frameListMap.end()) {
                                const TagLib::ID3v2::FrameList& frames = picFramesFound->second;
                                if (!frames.isEmpty()) {
                                    for (const TagLib::ID3v2::Frame* frame : frames) {
                                        const auto pictureFrame = static_cast<const TagLib::ID3v2::AttachedPictureFrame*>(frame);
                                        if (pictureFrame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
                                            setMediaArt(pictureFrame->picture());
                                            return;
                                        }
                                    }
                                    setMediaArt(static_cast<const TagLib::ID3v2::AttachedPictureFrame*>(frames.front())->picture());
                                }
                            }
                        }
                    }
                }

                void checkMimeType(QLatin1String mimeType) const
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
                            Info info{};
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

                void checkMimeType(QLatin1String) const
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
                    //processor.processFile(TagLib::EBML::Matroska::File(filePath.toUtf8()));
                    // Matroska support in TagLib seems to be buggy/incomplete
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
        const QLatin1String AlbumArtistsTag("ALBUMARTIST");
        const QLatin1String AlbumsTag("ALBUM");
        const QLatin1String YearTag("DATE");
        const QLatin1String TrackNumberTag("TRACKNUMBER");
        const QLatin1String GenresTag("GENRE");
        const QLatin1String DiscNumberTag("DISCNUMBER");

        Info getTrackInfo(const QString& filePath, Extension extension, const QMimeDatabase& mimeDb)
        {
            Info info{};
            info.filePath = filePath;
            processFile(filePath, extension, mimeDb, ExtractProcessor{info, mimeDb});
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
                const QStringList v(i.value().toStringList());
                for (const QString& value : v) {
                    values.append(toTString(value));
                }
                replaceProperties.insert(toTString(i.key()), values);
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
                processFile(filePath, LibraryUtils::extensionFromSuffix(QFileInfo(filePath).suffix()), mimeDb, SaveProcessor{filePath, replaceProperties, infos, mimeDb});
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
