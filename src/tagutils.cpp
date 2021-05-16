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

#include <algorithm>

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QMimeDatabase>

#include <aifffile.h>
#include <apefile.h>
#include <apetag.h>
#include <attachedpictureframe.h>
#include <flacfile.h>
#include <id3v2tag.h>
#include <mp4file.h>
#include <mpegfile.h>
#include <oggflacfile.h>
#include <opusfile.h>
#include <speexfile.h>
#include <tpropertymap.h>
#include <vorbisfile.h>
#include <wavfile.h>

#include "utilsfunctions.h"

namespace unplayer
{
    QLatin1String Tags::title()
    {
        return QLatin1String("TITLE");
    }

    QLatin1String Tags::artists()
    {
        return QLatin1String("ARTIST");
    }

    QLatin1String Tags::albumArtists()
    {
        return QLatin1String("ALBUMARTIST");
    }

    QLatin1String Tags::albums()
    {
        return QLatin1String("ALBUM");
    }

    QLatin1String Tags::year()
    {
        return QLatin1String("DATE");
    }

    QLatin1String Tags::trackNumber()
    {
        return QLatin1String("TRACKNUMBER");
    }

    QLatin1String Tags::genres()
    {
        return QLatin1String("GENRE");
    }

    QLatin1String Tags::discNumber()
    {
        return QLatin1String("DISCNUMBER");
    }

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

            const QLatin1String aacMimeType("audio/aac");
            const QLatin1String oggMimeType("application/ogg");
            const QLatin1String oggVorbisMimeType("audio/x-vorbis+ogg");
            const QLatin1String oggOpusMimeType("audio/x-opus+ogg");
            const QLatin1String oggSpeexMimeType("audio/x-speex+ogg");
            const QLatin1String oggFlacMimeType("audio/x-flac+ogg");

            template<typename ReturnValue>
            class Processor
            {
            public:
                virtual ~Processor() = default;

                std::optional<ReturnValue> process(const QString& filePath, fileutils::Extension extension)
                {
                    using namespace fileutils;

                    switch (extension) {
                    case Extension::FLAC:
                        return processFile(filePath, TagLib::FLAC::File(filePath.toUtf8()), AudioCodec::FLAC);
                    case Extension::AAC:
                        return processOnlyMimeType(filePath, aacMimeType, AudioCodec::AAC);
                    case Extension::M4A:
                    {
                        TagLib::MP4::File file(filePath.toUtf8());
                        auto audioCodec = AudioCodec::Unknown;
                        if (file.isValid()) {
                            switch (file.audioProperties()->codec()) {
                            case TagLib::MP4::Properties::AAC:
                                audioCodec = fileutils::AudioCodec::AAC;
                                break;
                            case TagLib::MP4::Properties::ALAC:
                                audioCodec = fileutils::AudioCodec::ALAC;
                                break;
                            case TagLib::MP4::Properties::Unknown:
                                break;
                            }
                        }
                        return processFile(filePath, std::move(file), audioCodec);
                    }
                    case Extension::MP3:
                        return processFile(filePath, TagLib::MPEG::File(filePath.toUtf8()), AudioCodec::MP3);
                    case Extension::OGG:
                    {
                        const QMimeType mimeType(mimeDb().mimeTypeForFile(filePath, QMimeDatabase::MatchContent));
                        const QString mimeTypeName(mimeType.name());
                        if (mimeTypeName == oggVorbisMimeType) {
                            return processFile(filePath, TagLib::Ogg::Vorbis::File(filePath.toUtf8()), AudioCodec::Vorbis);
                        } else if (mimeTypeName == oggOpusMimeType) {
                            return processFile(filePath, TagLib::Ogg::Opus::File(filePath.toUtf8()), AudioCodec::Opus);
                        } else if (mimeTypeName == oggSpeexMimeType) {
                            return processFile(filePath, TagLib::Ogg::Speex::File(filePath.toUtf8()), AudioCodec::Speex);
                        } else if (mimeTypeName == oggFlacMimeType) {
                            return processFile(filePath, TagLib::Ogg::FLAC::File(filePath.toUtf8()), AudioCodec::FLAC);
                        } else if (mimeType.inherits(oggMimeType)) {
                            error(filePath, errorCauseFormatNotSupported);
                        } else {
                            error(filePath, errorCauseExtensionDoesntMatch);
                        }
                        break;
                    }
                    case Extension::OPUS:
                        return processFile(filePath, TagLib::Ogg::Opus::File(filePath.toUtf8()), AudioCodec::Opus);
                    case Extension::SPX:
                        return processFile(filePath, TagLib::Ogg::Speex::File(filePath.toUtf8()), AudioCodec::Speex);
                    case Extension::APE:
                        return processFile(filePath, TagLib::APE::File(filePath.toUtf8()), AudioCodec::APE);
                    case Extension::WAV:
                        return processFile(filePath, TagLib::RIFF::WAV::File(filePath.toUtf8()), AudioCodec::LPCM);
                    case Extension::AIFF:
                    {
                        TagLib::RIFF::AIFF::File file(filePath.toUtf8());
                        auto audioCodec = AudioCodec::Unknown;
                        if (file.isValid()) {
                            if (file.audioProperties()->isAiffC()) {
                                audioCodec = AudioCodec::AIFFC;
                            } else {
                                audioCodec = AudioCodec::LPCM;
                            }
                        }
                        return processFile(filePath, std::move(file), audioCodec);
                    }
                    case Extension::Other:
                        error(filePath, errorCauseFormatNotSupported);
                        break;
                    }

                    return std::nullopt;
                }

            protected:
                virtual std::optional<ReturnValue> processBaseFile(const QString& filePath, TagLib::File& file, fileutils::AudioCodec audioCodec) = 0;
                virtual std::optional<ReturnValue> processFile(const QString& filePath, TagLib::FLAC::File&& file, fileutils::AudioCodec audioCodec) { return processBaseFile(filePath, file, audioCodec); }
                virtual std::optional<ReturnValue> processFile(const QString& filePath, TagLib::MP4::File&& file, fileutils::AudioCodec audioCodec) { return processBaseFile(filePath, file, audioCodec); }
                virtual std::optional<ReturnValue> processFile(const QString& filePath, TagLib::MPEG::File&& file, fileutils::AudioCodec audioCodec) { return processBaseFile(filePath, file, audioCodec); }
                virtual std::optional<ReturnValue> processFile(const QString& filePath, TagLib::Ogg::Vorbis::File&& file, fileutils::AudioCodec audioCodec) { return processBaseFile(filePath, file, audioCodec); }
                virtual std::optional<ReturnValue> processFile(const QString& filePath, TagLib::Ogg::Opus::File&& file, fileutils::AudioCodec audioCodec) { return processBaseFile(filePath, file, audioCodec); }
                virtual std::optional<ReturnValue> processFile(const QString& filePath, TagLib::Ogg::Speex::File&& file, fileutils::AudioCodec audioCodec) { return processBaseFile(filePath, file, audioCodec); }
                virtual std::optional<ReturnValue> processFile(const QString& filePath, TagLib::Ogg::FLAC::File&& file, fileutils::AudioCodec audioCodec) { return processBaseFile(filePath, file, audioCodec); }
                virtual std::optional<ReturnValue> processFile(const QString& filePath, TagLib::APE::File&& file, fileutils::AudioCodec audioCodec) { return processBaseFile(filePath, file, audioCodec); }
                virtual std::optional<ReturnValue> processFile(const QString& filePath, TagLib::RIFF::WAV::File&& file, fileutils::AudioCodec audioCodec) { return processBaseFile(filePath, file, audioCodec); }
                virtual std::optional<ReturnValue> processFile(const QString& filePath, TagLib::RIFF::AIFF::File&& file, fileutils::AudioCodec audioCodec) { return processBaseFile(filePath, file, audioCodec); }

                virtual std::optional<ReturnValue> processOnlyMimeType(const QString& filePath, QLatin1String mimeType, fileutils::AudioCodec audioCodec) = 0;

                virtual void error(const QString& filePath, const char* errorCause) = 0;

                QMimeDatabase& mimeDb() {
                    if (!mMimeDb) {
                        mMimeDb.emplace();
                    }
                    return *mMimeDb;
                }

            private:
                std::optional<QMimeDatabase> mMimeDb;
            };

            class ExtractProcessor : public Processor<Info>
            {
            protected:
                using Processor::processFile;

                std::optional<Info> processBaseFile(const QString& filePath, TagLib::File& file, fileutils::AudioCodec audioCodec) override
                {
                    if (!file.isValid()) {
                        error(file.name(), errorCauseExtensionDoesntMatch);
                        return std::nullopt;
                    }

                    Info info(filePath, audioCodec, true);

                    const TagLib::Tag* tag = file.tag();
                    const TagLib::PropertyMap properties(file.properties());

                    info.title = toQString(tag->title());
                    info.year = static_cast<int>(tag->year());
                    info.trackNumber = static_cast<int>(tag->track());

                    const TagLib::StringList& artists = properties[Tags::artists().data()];
                    info.artists.reserve(static_cast<int>(artists.size()));
                    for (const TagLib::String& artist : artists) {
                        const QString a(toQString(artist));
                        if (!a.isEmpty()) {
                            info.artists.push_back(a);
                        }
                    }
                    info.artists.removeDuplicates();

                    const TagLib::StringList& albumArtists = properties[Tags::albumArtists().data()];
                    info.albumArtists.reserve(static_cast<int>(albumArtists.size()));
                    for (const TagLib::String& albumArtist : albumArtists) {
                        const QString a(toQString(albumArtist));
                        if (!a.isEmpty()) {
                            info.albumArtists.push_back(a);
                        }
                    }
                    info.albumArtists.removeDuplicates();

                    const TagLib::StringList& albums = properties[Tags::albums().data()];
                    info.albums.reserve(static_cast<int>(albums.size()));
                    for (const TagLib::String& album : albums) {
                        const QString a(toQString(album));
                        if (!a.isEmpty()) {
                            info.albums.push_back(a);
                        }
                    }
                    info.albums.removeDuplicates();

                    const TagLib::StringList& genres = properties[Tags::genres().data()];
                    info.genres.reserve(static_cast<int>(genres.size()));
                    for (const TagLib::String& genre : genres) {
                        const QString g(unquote(toQString(genre)));
                        if (!g.isEmpty()) {
                            info.genres.push_back(g);
                        }
                    }
                    info.genres.removeDuplicates();

                    {
                        const auto found = properties.find(Tags::discNumber().data());
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
                    info.sampleRate = audioProperties->sampleRate();
                    info.channels = audioProperties->channels();

                    return info;
                }

                std::optional<Info> processFile(const QString& filePath, TagLib::FLAC::File&& file, fileutils::AudioCodec audioCodec) override
                {
                    auto info = processBaseFile(filePath, file, audioCodec);
                    if (info) {
                        info->bitDepth = file.audioProperties()->bitsPerSample();

                        setMediaArtFromFlacPictures(*info, file.pictureList());
                        if (info->mediaArtData.isEmpty() && file.hasID3v2Tag()) {
                            setMediaArtFromIDv2Tag(*info, file.ID3v2Tag());
                        }
                    }
                    return info;
                }

                std::optional<Info> processFile(const QString& filePath, TagLib::MP4::File&& file, fileutils::AudioCodec audioCodec) override
                {
                    auto info = processBaseFile(filePath, file, audioCodec);
                    if (info) {
                        info->bitDepth = file.audioProperties()->bitsPerSample();

                        const TagLib::MP4::ItemMap& items = file.tag()->itemMap();
                        const auto found(items.find("covr"));
                        if (found != items.end()) {
                            const TagLib::MP4::CoverArtList covers(found->second.toCoverArtList());
                            if (!covers.isEmpty()) {
                                setMediaArt(*info, covers.front().data());
                            }
                        }
                    }
                    return info;
                }

                std::optional<Info> processFile(const QString& filePath, TagLib::MPEG::File&& file, fileutils::AudioCodec audioCodec) override
                {
                    auto info = processBaseFile(filePath, file, audioCodec);
                    if (info) {
                        if (file.hasID3v2Tag()) {
                            setMediaArtFromIDv2Tag(*info, file.ID3v2Tag());
                        }
                        if (info->mediaArtData.isEmpty() && file.hasAPETag()) {
                            setMediaArtFromApeTag(*info, file.APETag());
                        }
                    }
                    return info;
                }

                std::optional<Info> processFile(const QString& filePath, TagLib::Ogg::Vorbis::File&& file, fileutils::AudioCodec audioCodec) override
                {
                    auto info = processBaseFile(filePath, file, audioCodec);
                    if (info) {
                        setMediaArtFromFlacPictures(*info, file.tag()->pictureList());
                    }
                    return info;
                }

                std::optional<Info> processFile(const QString& filePath, TagLib::Ogg::Opus::File&& file, fileutils::AudioCodec audioCodec) override
                {
                    auto info = processBaseFile(filePath, file, audioCodec);
                    if (info) {
                        setMediaArtFromFlacPictures(*info, file.tag()->pictureList());
                    }
                    return info;
                }

                std::optional<Info> processFile(const QString& filePath, TagLib::Ogg::Speex::File&& file, fileutils::AudioCodec audioCodec) override
                {
                    auto info = processBaseFile(filePath, file, audioCodec);
                    if (info) {
                        setMediaArtFromFlacPictures(*info, file.tag()->pictureList());
                    }
                    return info;
                }

                std::optional<Info> processFile(const QString& filePath, TagLib::Ogg::FLAC::File&& file, fileutils::AudioCodec audioCodec) override
                {
                    auto info = processBaseFile(filePath, file, audioCodec);
                    if (info) {
                        info->bitDepth = file.audioProperties()->bitsPerSample();
                        if (file.hasXiphComment()) {
                            setMediaArtFromFlacPictures(*info, file.tag()->pictureList());
                        }
                    }
                    return info;
                }

                std::optional<Info> processFile(const QString& filePath, TagLib::APE::File&& file, fileutils::AudioCodec audioCodec) override
                {
                    auto info = processBaseFile(filePath, file, audioCodec);
                    if (info) {
                        info->bitDepth = file.audioProperties()->bitsPerSample();
                        if (file.hasAPETag()) {
                            setMediaArtFromApeTag(*info, file.APETag());
                        }
                    }
                    return info;
                }

                std::optional<Info> processFile(const QString& filePath, TagLib::RIFF::WAV::File&& file, fileutils::AudioCodec audioCodec) override
                {
                    auto info = processBaseFile(filePath, file, audioCodec);
                    if (info) {
                        info->bitDepth = file.audioProperties()->bitsPerSample();
                        if (file.hasID3v2Tag()) {
                            setMediaArtFromIDv2Tag(*info, file.ID3v2Tag());
                        }
                    }
                    return info;
                }

                std::optional<Info> processFile(const QString& filePath, TagLib::RIFF::AIFF::File&& file, fileutils::AudioCodec audioCodec) override
                {
                    auto info = processBaseFile(filePath, file, audioCodec);
                    if (info) {
                        info->bitDepth = file.audioProperties()->bitsPerSample();
                        if (file.hasID3v2Tag()) {
                            setMediaArtFromIDv2Tag(*info, file.tag());
                        }
                    }
                    return info;
                }

                std::optional<Info> processOnlyMimeType(const QString& filePath, QLatin1String mimeType, fileutils::AudioCodec audioCodec) override
                {
                    if (mimeDb().mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name() == mimeType) {
                        error(filePath, errorCauseTagExtractionNotSupported);
                        return Info(filePath, audioCodec, false);
                    }
                    return std::nullopt;
                }

                void error(const QString& filePath, const char* errorCause) override
                {
                    qWarning().nospace() << "Warning: can't read tags from " << filePath << ": " << errorCause;
                }

            private:
                void setMediaArt(Info& info, const TagLib::ByteVector& data) const
                {
                    info.mediaArtData = QByteArray(data.data(), static_cast<int>(data.size()));
                }

                void setMediaArtFromFlacPictures(Info& info, const TagLib::List<TagLib::FLAC::Picture*>& pictures) const
                {
                    if (!pictures.isEmpty()) {
                        const TagLib::FLAC::Picture* backCover = nullptr;
                        for (const TagLib::FLAC::Picture* picture : pictures) {
                            if (picture->type() == TagLib::FLAC::Picture::FrontCover) {
                                setMediaArt(info, picture->data());
                                return;
                            }
                            if (!backCover && picture->type() == TagLib::FLAC::Picture::BackCover) {
                                backCover = picture;
                            }
                        }
                        if (backCover) {
                            setMediaArt(info, backCover->data());
                        } else {
                            setMediaArt(info, pictures.front()->data());
                        }
                    }
                }

                void setMediaArtFromIDv2Tag(Info& info, const TagLib::ID3v2::Tag* tag) const
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
                                        setMediaArt(info, pictureFrame->picture());
                                        return;
                                    }
                                    if (!backCover && pictureFrame->type() == TagLib::ID3v2::AttachedPictureFrame::BackCover) {
                                        backCover = pictureFrame;
                                    }
                                }
                                if (backCover) {
                                    setMediaArt(info, backCover->picture());
                                } else {
                                    setMediaArt(info, static_cast<const TagLib::ID3v2::AttachedPictureFrame*>(frames.front())->picture());
                                }
                            }
                        }
                    };
                    setFromFrames("APIC");
                    if (info.mediaArtData.isEmpty()) {
                        setFromFrames("PIC");
                    }
                }

                void setMediaArtFromApeTag(Info& info, const TagLib::APE::Tag* tag) const
                {
                    const TagLib::APE::ItemListMap& items = tag->itemListMap();
                    const auto setFromItem = [&](const char* itemTag) {
                        const auto found(items.find(itemTag));
                        if (found != items.end()) {
                            const auto& item = found->second;
                            if (item.type() == TagLib::APE::Item::Binary) {
                                const TagLib::ByteVector data(found->second.binaryData());
                                setMediaArt(info, data.mid(static_cast<unsigned int>(data.find('\0') + 1)));
                            }
                        }
                    };
                    setFromItem("COVER ART (FRONT)");
                    if (info.mediaArtData.isEmpty()) {
                        setFromItem("COVER ART (BACK)");
                    }
                }
            };

            class AudioFormatProcessor final : public Processor<AudioCodecInfo>
            {
                using Processor::processFile;

                std::optional<AudioCodecInfo> processFile(const QString& filePath, TagLib::FLAC::File&& file, fileutils::AudioCodec audioCodec) override
                {
                    auto info = processBaseFile(filePath, file, audioCodec);
                    if (info) {
                        info->bitDepth = file.audioProperties()->bitsPerSample();
                    }
                    return info;
                }

                std::optional<AudioCodecInfo> processFile(const QString& filePath, TagLib::MP4::File&& file, fileutils::AudioCodec audioCodec) override
                {
                    auto info = processBaseFile(filePath, file, audioCodec);
                    if (info) {
                        info->bitDepth = file.audioProperties()->bitsPerSample();
                    }
                    return info;
                }

                std::optional<AudioCodecInfo> processFile(const QString& filePath, TagLib::Ogg::FLAC::File&& file, fileutils::AudioCodec audioCodec) override
                {
                    auto info = processBaseFile(filePath, file, audioCodec);
                    if (info) {
                        info->bitDepth = file.audioProperties()->bitsPerSample();
                    }
                    return info;
                }

                std::optional<AudioCodecInfo> processFile(const QString& filePath, TagLib::APE::File&& file, fileutils::AudioCodec audioCodec) override
                {
                    auto info = processBaseFile(filePath, file, audioCodec);
                    if (info) {
                        info->bitDepth = file.audioProperties()->bitsPerSample();
                    }
                    return info;
                }

                std::optional<AudioCodecInfo> processFile(const QString& filePath, TagLib::RIFF::WAV::File&& file, fileutils::AudioCodec audioCodec) override
                {
                    auto info = processBaseFile(filePath, file, audioCodec);
                    if (info) {
                        info->bitDepth = file.audioProperties()->bitsPerSample();
                    }
                    return info;
                }

                std::optional<AudioCodecInfo> processFile(const QString& filePath, TagLib::RIFF::AIFF::File&& file, fileutils::AudioCodec audioCodec) override
                {
                    auto info = processBaseFile(filePath, file, audioCodec);
                    if (info) {
                        info->bitDepth = file.audioProperties()->bitsPerSample();
                    }
                    return info;
                }

                std::optional<AudioCodecInfo> processBaseFile(const QString& filePath, TagLib::File& file, fileutils::AudioCodec audioCodec) override
                {
                    if (file.isValid()) {
                        AudioCodecInfo info(audioCodec);
                        const auto audioProperties = file.audioProperties();
                        info.sampleRate = audioProperties->sampleRate();
                        info.bitrate = audioProperties->bitrate();
                        return info;
                    }
                    error(filePath, errorCauseExtensionDoesntMatch);
                    return std::nullopt;
                }

                std::optional<AudioCodecInfo> processOnlyMimeType(const QString& filePath, QLatin1String mimeType, fileutils::AudioCodec audioCodec) override
                {
                    if (audioCodec != fileutils::AudioCodec::Unknown) {
                        if (mimeDb().mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name() == mimeType) {
                            return AudioCodecInfo(audioCodec);
                        }
                    }
                    return std::nullopt;
                }

                void error(const QString& filePath, const char* errorCause) override
                {
                    qWarning().nospace() << "Warning: can't extract bits per sample for file:" << filePath << ": " << errorCause;
                }
            };

            class SaveProcessor : public ExtractProcessor
            {
            public:
                SaveProcessor(const TagLib::PropertyMap& replaceProperties)
                    : mReplaceProperties(replaceProperties)
                {

                }
            private:
                using Processor::processFile;
                using ExtractProcessor::processFile;

                std::optional<Info> processOnlyMimeType(const QString& filePath, QLatin1String, fileutils::AudioCodec) override
                {
                    error(filePath, errorCauseTagSavingNotSupported);
                    return std::nullopt;
                }

                void error(const QString& filePath, const char* errorCause) override
                {
                    qWarning().nospace() << "Warning: can't save tags for file " << filePath << ": " << errorCause;
                }

                std::optional<Info> processBaseFile(const QString& filePath, TagLib::File& file, fileutils::AudioCodec audioCodec) override
                {
                    if (file.isValid()) {
                        TagLib::PropertyMap properties(file.properties());
                        for (const auto& i : mReplaceProperties) {
                            properties.replace(i.first, i.second);
                        }
                        file.setProperties(properties);
                        if (file.save()) {
                            return ExtractProcessor::processBaseFile(filePath, file, audioCodec);
                        }
                        error(filePath, "error when saving file");
                    } else {
                        error(filePath, errorCauseExtensionDoesntMatch);
                    }
                    return std::nullopt;
                }

                const TagLib::PropertyMap& mReplaceProperties;
            };
        }

        std::optional<Info> getTrackInfo(const QString& filePath, fileutils::Extension extension)
        {
            return ExtractProcessor().process(filePath, extension);
        }

        std::optional<QByteArray> getTackMediaArtData(const QString &filePath, fileutils::Extension extension)
        {
            if (auto info = ExtractProcessor().process(filePath, extension); info) {
                return std::move(info->mediaArtData);
            }
            return std::nullopt;
        }

        std::optional<AudioCodecInfo> getTrackAudioCodecInfo(const QString& filePath, fileutils::Extension extension)
        {
            return AudioFormatProcessor().process(filePath, extension);
        }

        template<bool IncrementTrackNumber>
        std::vector<Info> saveTags(const QStringList& files, const QVariantMap& tags, const std::function<void(Info&)>& callback)
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
                trackNumberString = &(replaceProperties[Tags::trackNumber().data()][0]);
                trackNumber = trackNumberString->toInt();
            }

            SaveProcessor processor(replaceProperties);

            for (const QString& filePath : files) {
                if (!qApp) {
                    break;
                }

                auto info = processor.process(filePath, fileutils::extensionFromSuffix(QFileInfo(filePath).suffix()));
                if (info) {
                    callback(*info);
                    infos.push_back(std::move(*info));

                    if (IncrementTrackNumber) {
                        ++trackNumber;
                        *trackNumberString = TagLib::String::number(trackNumber);
                    }
                }
            }

            return infos;
        }

        template std::vector<Info> saveTags<true>(const QStringList& files, const QVariantMap& tags, const std::function<void(Info&)>& callback);
        template std::vector<Info> saveTags<false>(const QStringList& files, const QVariantMap& tags, const std::function<void(Info&)>& callback);
    }
}
