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

#include "tagutils.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QFileInfo>
#include <QMimeDatabase>

#include <apefile.h>
#include <apetag.h>
#include <attachedpictureframe.h>
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

            inline QString unquote(QString str) {
                static const QLatin1Char quoteChar('"');
                if (str.startsWith(quoteChar) && str.endsWith(quoteChar) && str.size() > 1) {
                    str.chop(1);
                    str.remove(0, 1);
                }
                return str;
            }

            inline QString toQString(const TagLib::String& string)
            {
                if (!string.isEmpty() && string[0] == QChar::ByteOrderMark) {
                    std::vector<char16_t> vec(string.begin(), string.end());
                    QString str(QString::fromUtf16(vec.data(), static_cast<int>(vec.size())));
                    return str.trimmed();
                }
                QString str(static_cast<int>(string.size()), QChar());
                std::copy(string.begin(), string.end(), str.begin());
                return str.trimmed();
            }

            const char* errorCauseTagExtractionNotSupported = "tag extraction for this file format is not supported";
            const char* errorCauseTagSavingNotSupported = "tag saving for this file format is not supported";
            const char* errorCauseExtensionDoesntMatch = "file format doesn't match extension";
            const char* errorCauseFormatNotSupported = "file format is not supported";

            struct ExtractProcessor
            {
                void setMediaArt(const TagLib::ByteVector& data) const
                {
                    info.mediaArtData = QByteArray(data.data(), static_cast<int>(data.size()));
                }

                void setMediaArtFromFlacPictures(const TagLib::List<TagLib::FLAC::Picture*>& pictures) const
                {
                    if (!pictures.isEmpty()) {
                        const TagLib::FLAC::Picture* backCover = nullptr;
                        for (const TagLib::FLAC::Picture* picture : pictures) {
                            if (picture->type() == TagLib::FLAC::Picture::FrontCover) {
                                setMediaArt(picture->data());
                                return;
                            }
                            if (!backCover && picture->type() == TagLib::FLAC::Picture::BackCover) {
                                backCover = picture;
                            }
                        }
                        if (backCover) {
                            setMediaArt(backCover->data());
                        } else {
                            setMediaArt(pictures.front()->data());
                        }
                    }
                }

                void setMediaArtFromIDv2Tag(const TagLib::ID3v2::Tag* tag) const
                {
                    const TagLib::ID3v2::FrameListMap& frameListMap = tag->frameListMap();
                    const auto setFromFrames = [&](const char* framesTag) {
                        const auto framesFound(frameListMap.find(framesTag));
                        if (framesFound != frameListMap.end()) {
                            const TagLib::ID3v2::FrameList& frames = framesFound->second;
                            if (!frames.isEmpty()) {
                                const TagLib::ID3v2::AttachedPictureFrame* backCover = nullptr;
                                for (const TagLib::ID3v2::Frame* frame : frames) {
                                    const auto pictureFrame = static_cast<const TagLib::ID3v2::AttachedPictureFrame*>(frame);
                                    if (pictureFrame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
                                        setMediaArt(pictureFrame->picture());
                                        return;
                                    }
                                    if (!backCover && pictureFrame->type() == TagLib::ID3v2::AttachedPictureFrame::BackCover) {
                                        backCover = pictureFrame;
                                    }
                                }
                                if (backCover) {
                                    setMediaArt(backCover->picture());
                                } else {
                                    setMediaArt(static_cast<const TagLib::ID3v2::AttachedPictureFrame*>(frames.front())->picture());
                                }
                            }
                        }
                    };
                    setFromFrames("APIC");
                    if (info.mediaArtData.isEmpty()) {
                        setFromFrames("PIC");
                    }
                }

                void setMediaArtFromApeTag(const TagLib::APE::Tag* tag) const
                {
                    const TagLib::APE::ItemListMap& items = tag->itemListMap();
                    const auto setFromItem = [&](const char* itemTag) {
                        const auto found(items.find(itemTag));
                        if (found != items.end()) {
                            const auto& item = found->second;
                            if (item.type() == TagLib::APE::Item::Binary) {
                                const TagLib::ByteVector data(found->second.binaryData());
                                setMediaArt(data.mid(static_cast<unsigned int>(data.find('\0') + 1)));
                            }
                        }
                    };
                    setFromItem("COVER ART (FRONT)");
                    if (info.mediaArtData.isEmpty()) {
                        setFromItem("COVER ART (BACK)");
                    }
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
                            const QString a(toQString(artist));
                            if (!a.isEmpty()) {
                                info.artists.push_back(a);
                            }
                        }
                        info.artists.removeDuplicates();

                        const TagLib::StringList& albumArtists = properties[AlbumArtistsTag.data()];
                        info.albumArtists.reserve(static_cast<int>(albumArtists.size()));
                        for (const TagLib::String& albumArtist : albumArtists) {
                            const QString a(toQString(albumArtist));
                            if (!a.isEmpty()) {
                                info.albumArtists.push_back(a);
                            }
                        }
                        info.albumArtists.removeDuplicates();

                        const TagLib::StringList& albums = properties[AlbumsTag.data()];
                        info.albums.reserve(static_cast<int>(albums.size()));
                        for (const TagLib::String& album : albums) {
                            const QString a(toQString(album));
                            if (!a.isEmpty()) {
                                info.albums.push_back(a);
                            }
                        }
                        info.albums.removeDuplicates();

                        const TagLib::StringList& genres = properties[GenresTag.data()];
                        info.genres.reserve(static_cast<int>(genres.size()));
                        for (const TagLib::String& genre : genres) {
                            const QString g(unquote(toQString(genre)));
                            if (!g.isEmpty()) {
                                info.genres.push_back(g);
                            }
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
                        setMediaArtFromFlacPictures(file.pictureList());
                        if (info.mediaArtData.isEmpty() && file.hasID3v2Tag()) {
                            setMediaArtFromIDv2Tag(file.ID3v2Tag());
                        }
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
                        if (file.hasID3v2Tag()) {
                            setMediaArtFromIDv2Tag(file.ID3v2Tag());
                        }
                        if (info.mediaArtData.isEmpty() && file.hasAPETag()) {
                            setMediaArtFromApeTag(file.APETag());
                        }
                    }
                }

                void processFile(TagLib::Ogg::Vorbis::File&& file) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file))) {
                        setMediaArtFromFlacPictures(file.tag()->pictureList());
                    }
                }

                void processFile(TagLib::Ogg::Opus::File&& file) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file))) {
                        setMediaArtFromFlacPictures(file.tag()->pictureList());
                    }
                }

                void processFile(TagLib::Ogg::FLAC::File&& file) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file))) {
                        if (file.hasXiphComment()) {
                            setMediaArtFromFlacPictures(file.tag()->pictureList());
                        }
                    }
                }

                void processFile(TagLib::APE::File&& file) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file))) {
                        if (file.hasAPETag()) {
                            setMediaArtFromApeTag(file.APETag());
                        }
                    }
                }

                void processFile(TagLib::RIFF::WAV::File&& file) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file))) {
                        if (file.hasID3v2Tag()) {
                            setMediaArtFromIDv2Tag(file.ID3v2Tag());
                        }
                    }
                }

                void processFile(TagLib::WavPack::File&& file) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file))) {
                        if (file.hasAPETag()) {
                            setMediaArtFromApeTag(file.APETag());
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
                template<typename File>
                void processFile(File&& file) const
                {
                    if (saveProperties(std::forward<File>(file))) {
                        ExtractProcessor{info, mimeDb}.processFile(std::forward<File>(file));
                    }
                }

                bool saveProperties(TagLib::File&& file) const
                {
                    if (file.isValid()) {
                        TagLib::PropertyMap properties(file.properties());
                        for (const auto& i : replaceProperties) {
                            properties.replace(i.first, i.second);
                        }
                        file.setProperties(properties);
                        if (file.save()) {
                            return true;
                        }
                        error("error when saving file");
                    } else {
                        error(errorCauseExtensionDoesntMatch);
                    }
                    return false;
                }

                void checkMimeType(QLatin1String) const
                {
                    error(errorCauseTagSavingNotSupported);
                }

                void error(const char* errorCause) const
                {
                    qWarning().nospace() << "Warning: can't save tags for file " << info.filePath << ": " << errorCause;
                }

                Info& info;
                const TagLib::PropertyMap& replaceProperties;
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
        std::vector<Info> saveTags(const QStringList& files, const QVariantMap& tags, const QMimeDatabase& mimeDb, const std::function<void(Info&)>& callback)
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
                Info info{};
                info.filePath = filePath;
                processFile(filePath, LibraryUtils::extensionFromSuffix(QFileInfo(filePath).suffix()), mimeDb, SaveProcessor{info, replaceProperties, mimeDb});

                if (info.canReadTags) {
                    callback(info);
                    infos.push_back(std::move(info));

                    if (IncrementTrackNumber) {
                        ++trackNumber;
                        *trackNumberString = TagLib::String::number(trackNumber);
                    }
                }
            }

            return infos;
        }

        template std::vector<Info> saveTags<true>(const QStringList& files, const QVariantMap& tags, const QMimeDatabase& mimeDb, const std::function<void(Info&)>& callback);
        template std::vector<Info> saveTags<false>(const QStringList& files, const QVariantMap& tags, const QMimeDatabase& mimeDb, const std::function<void(Info&)>& callback);
    }
}
