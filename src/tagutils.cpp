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

            class ExtractProcessor
            {
            public:
                explicit ExtractProcessor(Info& info, const QMimeDatabase& mimeDb)
                    : info(info), mMimeDb(mimeDb)
                {

                }

                bool processFile(TagLib::File&& file, fileutils::AudioCodec audioCodec) const
                {
                    if (file.isValid()) {
                        info.audioCodec = audioCodec;
                        info.fileTypeMatchesExtension = true;
                        info.canReadTags = true;

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
                    } else {
                        error(errorCauseExtensionDoesntMatch);
                    }
                    return info.fileTypeMatchesExtension;
                }

                void processFile(TagLib::FLAC::File&& file, fileutils::AudioCodec audioCodec) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file), audioCodec)) {
                        info.bitDepth = file.audioProperties()->bitsPerSample();

                        setMediaArtFromFlacPictures(file.pictureList());
                        if (info.mediaArtData.isEmpty() && file.hasID3v2Tag()) {
                            setMediaArtFromIDv2Tag(file.ID3v2Tag());
                        }
                    }
                }

                void processFile(TagLib::MP4::File&& file, fileutils::AudioCodec audioCodec) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file), audioCodec)) {
                        info.bitDepth = file.audioProperties()->bitsPerSample();

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

                void processFile(TagLib::MPEG::File&& file, fileutils::AudioCodec audioCodec) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file), audioCodec)) {
                        if (file.hasID3v2Tag()) {
                            setMediaArtFromIDv2Tag(file.ID3v2Tag());
                        }
                        if (info.mediaArtData.isEmpty() && file.hasAPETag()) {
                            setMediaArtFromApeTag(file.APETag());
                        }
                    }
                }

                void processFile(TagLib::Ogg::Vorbis::File&& file, fileutils::AudioCodec audioCodec) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file), audioCodec)) {
                        setMediaArtFromFlacPictures(file.tag()->pictureList());
                    }
                }

                void processFile(TagLib::Ogg::Opus::File&& file, fileutils::AudioCodec audioCodec) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file), audioCodec)) {
                        setMediaArtFromFlacPictures(file.tag()->pictureList());
                    }
                }

                void processFile(TagLib::Ogg::FLAC::File&& file, fileutils::AudioCodec audioCodec) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file), audioCodec)) {
                        info.bitDepth = file.audioProperties()->bitsPerSample();
                        if (file.hasXiphComment()) {
                            setMediaArtFromFlacPictures(file.tag()->pictureList());
                        }
                    }
                }

                void processFile(TagLib::APE::File&& file, fileutils::AudioCodec audioCodec) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file), audioCodec)) {
                        info.bitDepth = file.audioProperties()->bitsPerSample();
                        if (file.hasAPETag()) {
                            setMediaArtFromApeTag(file.APETag());
                        }
                    }
                }

                void processFile(TagLib::RIFF::WAV::File&& file, fileutils::AudioCodec audioCodec) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file), audioCodec)) {
                        info.bitDepth = file.audioProperties()->bitsPerSample();
                        if (file.hasID3v2Tag()) {
                            setMediaArtFromIDv2Tag(file.ID3v2Tag());
                        }
                    }
                }

                void processFile(TagLib::WavPack::File&& file, fileutils::AudioCodec audioCodec) const
                {
                    if (processFile(static_cast<TagLib::File&&>(file), audioCodec)) {
                        info.bitDepth = file.audioProperties()->bitsPerSample();
                        if (file.hasAPETag()) {
                            setMediaArtFromApeTag(file.APETag());
                        }
                    }
                }

                void processOnlyMimeType(QLatin1String mimeType, fileutils::AudioCodec audioCodec) const
                {
                    if (mMimeDb.mimeTypeForFile(info.filePath, QMimeDatabase::MatchContent).name() == mimeType) {
                        error(errorCauseTagExtractionNotSupported);
                        info.audioCodec = audioCodec;
                        info.fileTypeMatchesExtension = true;
                    } else {
                        error(errorCauseExtensionDoesntMatch);
                    }
                }

                const QMimeDatabase& mimeDb() const
                {
                    return mMimeDb;
                }

                void error(const char* errorCause) const
                {
                    qWarning().nospace() << "Warning: can't read tags from " << info.filePath << ": " << errorCause;
                }

                Info& info;

            private:
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

                const QMimeDatabase& mMimeDb;
            };

            struct AudioFormatProcessor
            {
                void processFile(TagLib::FLAC::File&& file, fileutils::AudioCodec audioCodec)
                {
                    if (processFile(static_cast<TagLib::File&&>(file), audioCodec)) {
                        info.bitDepth = file.audioProperties()->bitsPerSample();
                    }
                }

                void processFile(TagLib::MP4::File&& file, fileutils::AudioCodec audioCodec)
                {
                    if (processFile(static_cast<TagLib::File&&>(file), audioCodec)) {
                        info.bitDepth = file.audioProperties()->bitsPerSample();
                    }
                }

                void processFile(TagLib::Ogg::FLAC::File&& file, fileutils::AudioCodec audioCodec)
                {
                    if (processFile(static_cast<TagLib::File&&>(file), audioCodec)) {
                        info.bitDepth = file.audioProperties()->bitsPerSample();
                    }
                }

                void processFile(TagLib::APE::File&& file, fileutils::AudioCodec audioCodec)
                {
                    if (processFile(static_cast<TagLib::File&&>(file), audioCodec)) {
                        info.bitDepth = file.audioProperties()->bitsPerSample();
                    }
                }

                void processFile(TagLib::RIFF::WAV::File&& file, fileutils::AudioCodec audioCodec)
                {
                    if (processFile(static_cast<TagLib::File&&>(file), audioCodec)) {
                        info.bitDepth = file.audioProperties()->bitsPerSample();
                    }
                }

                void processFile(TagLib::WavPack::File&& file, fileutils::AudioCodec audioCodec)
                {
                    if (processFile(static_cast<TagLib::File&&>(file), audioCodec)) {
                        info.bitDepth = file.audioProperties()->bitsPerSample();
                    }
                }

                bool processFile(TagLib::File&& file, fileutils::AudioCodec audioCodec)
                {
                    if (file.isValid()) {
                        info.audioCodec = audioCodec;
                        const auto audioProperties = file.audioProperties();
                        info.sampleRate = audioProperties->sampleRate();
                        info.bitrate = audioProperties->bitrate();
                        return true;
                    }
                    error(errorCauseExtensionDoesntMatch);
                    return false;
                }

                void processOnlyMimeType(QLatin1String mimeType, fileutils::AudioCodec audioCodec) const
                {
                    if (audioCodec != fileutils::AudioCodec::Unknown) {
                        if (mimeDb().mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name() == mimeType) {
                            info.audioCodec = audioCodec;
                        }
                    }
                }

                QMimeDatabase mimeDb() const
                {
                    return {};
                }

                void error(const char* errorCause) const
                {
                    qWarning().nospace() << "Warning: can't extract bits per sample for file:" << filePath << ": " << errorCause;
                }

                QString filePath;
                AudioCodecInfo& info;
            };

            class SaveProcessor
            {
            public:
                SaveProcessor(Info& info, const TagLib::PropertyMap& replaceProperties, const QMimeDatabase& mimeDb)
                    : info(info), mReplaceProperties(replaceProperties), mMimeDb(mimeDb)
                {

                }

                template<typename File>
                void processFile(File&& file, fileutils::AudioCodec audioCodec) const
                {
                    if (saveProperties(file)) {
                        ExtractProcessor(info, mMimeDb).processFile(std::forward<File>(file), audioCodec);
                    }
                }

                const QMimeDatabase& mimeDb() const
                {
                    return mMimeDb;
                }

                void processOnlyMimeType(QLatin1String, fileutils::AudioCodec) const
                {
                    error(errorCauseTagSavingNotSupported);
                }

                void error(const char* errorCause) const
                {
                    qWarning().nospace() << "Warning: can't save tags for file " << info.filePath << ": " << errorCause;
                }

                Info& info;

            private:
                bool saveProperties(TagLib::File& file) const
                {
                    if (file.isValid()) {
                        TagLib::PropertyMap properties(file.properties());
                        for (const auto& i : mReplaceProperties) {
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

                const TagLib::PropertyMap& mReplaceProperties;
                const QMimeDatabase& mMimeDb;
            };

            const QLatin1String aacMimeType("audio/aac");
            const QLatin1String oggMimeType("application/ogg");
            const QLatin1String oggVorbisMimeType("audio/x-vorbis+ogg");
            const QLatin1String oggOpusMimeType("audio/x-opus+ogg");
            const QLatin1String oggFlacMimeType("audio/x-flac+ogg");
            const QLatin1String matroskaMimeType("application/x-matroska");

            template<class Processor>
            void processFile(const QString& filePath, fileutils::Extension extension, Processor& processor)
            {
                using namespace fileutils;
                switch (extension) {
                case Extension::FLAC:
                    processor.processFile(TagLib::FLAC::File(filePath.toUtf8()), AudioCodec::FLAC);
                    break;
                case Extension::AAC:
                    processor.processOnlyMimeType(aacMimeType, AudioCodec::AAC);
                    break;
                case Extension::M4A:
                {
                    TagLib::MP4::File file(filePath.toUtf8());
                    auto audioCodec = AudioCodec::Unknown;
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
                    processor.processFile(std::move(file), audioCodec);
                    break;
                }
                case Extension::MP3:
                    processor.processFile(TagLib::MPEG::File(filePath.toUtf8()), AudioCodec::MP3);
                    break;
                case Extension::OGG:
                {
                    const QMimeType mimeType(processor.mimeDb().mimeTypeForFile(filePath, QMimeDatabase::MatchContent));
                    const QString mimeTypeName(mimeType.name());
                    if (mimeTypeName == oggVorbisMimeType) {
                        processor.processFile(TagLib::Ogg::Vorbis::File(filePath.toUtf8()), AudioCodec::Vorbis);
                    } else if (mimeTypeName == oggOpusMimeType) {
                        processor.processFile(TagLib::Ogg::Opus::File(filePath.toUtf8()), AudioCodec::Opus);
                    } else if (mimeTypeName == oggFlacMimeType) {
                        processor.processFile(TagLib::Ogg::FLAC::File(filePath.toUtf8()), AudioCodec::FLAC);
                    } else if (mimeType.inherits(oggMimeType)) {
                        processor.error(errorCauseFormatNotSupported);
                    } else {
                        processor.error(errorCauseExtensionDoesntMatch);
                    }
                    break;
                }
                case Extension::OPUS:
                    processor.processFile(TagLib::Ogg::Opus::File(filePath.toUtf8()), AudioCodec::Opus);
                    break;
                case Extension::APE:
                    processor.processFile(TagLib::APE::File(filePath.toUtf8()), AudioCodec::APE);
                    break;
                case Extension::MKA:
                    processor.processOnlyMimeType(matroskaMimeType, AudioCodec::Unknown);
                    break;
                case Extension::WAV:
                    processor.processFile(TagLib::RIFF::WAV::File(filePath.toUtf8()), AudioCodec::LPCM);
                    break;
                case Extension::WAVPACK:
                    processor.processFile(TagLib::WavPack::File(filePath.toUtf8()), AudioCodec::WAVPACK);
                    break;
                case Extension::Other:
                    processor.error(errorCauseFormatNotSupported);
                    break;
                }
            }
        }

        Info getTrackInfo(const QString& filePath, fileutils::Extension extension, const QMimeDatabase& mimeDb)
        {
            Info info{};
            info.filePath = filePath;
            ExtractProcessor processor(info, mimeDb);
            processFile(filePath, extension, processor);
            return info;
        }

        AudioCodecInfo getTrackAudioCodecInfo(const QString& filePath, fileutils::Extension extension)
        {
            AudioCodecInfo info{};
            AudioFormatProcessor processor{filePath, info};
            processFile(filePath, extension, processor);
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
                trackNumberString = &(replaceProperties[Tags::trackNumber().data()][0]);
                trackNumber = trackNumberString->toInt();
            }

            for (const QString& filePath : files) {
                if (!qApp) {
                    break;
                }
                Info info{};
                info.filePath = filePath;
                SaveProcessor processor(info, replaceProperties, mimeDb);
                processFile(filePath, fileutils::extensionFromSuffix(QFileInfo(filePath).suffix()), processor);

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
