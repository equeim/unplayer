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

#include "commandlineparser.h"

#include <iostream>

#include <QFileInfo>
#include <QUrl>

#define CXXOPTS_VECTOR_DELIMITER '\0'
#include <cxxopts.hpp>

#include "fileutils.h"
#include "playlistutils.h"

namespace unplayer
{
    namespace
    {
        void parseFiles(const std::vector<std::string>& files, CommandLineArgs& args)
        {
            if (!files.empty()) {
                args.files.reserve(static_cast<int>(files.size()));
                for (const std::string& f : files) {
                    const auto file(QString::fromStdString(f));
                    const QFileInfo info(file);
                    if (info.isFile()) {
                        const QString suffix(info.suffix());
                        if (PlaylistUtils::isPlaylistExtension(suffix) ||
                            fileutils::isExtensionSupported(suffix) ||
                            fileutils::isVideoExtensionSupported(suffix)) {

                            args.files.push_back(QUrl::fromLocalFile(info.absoluteFilePath()).toString());
                        }
                    } else {
                        const QUrl url(file);
                        if (url.isLocalFile()) {
                            if (QFileInfo(url.path()).isFile()) {
                                args.files.push_back(file);
                            }
                        } else {
                            args.files.push_back(file);
                        }
                    }
                }
            }
        }
    }

    CommandLineArgs CommandLineArgs::parse(int& argc, char**& argv)
    {
        CommandLineArgs args{};

        const std::string appName(QFileInfo(*argv).fileName().toStdString());
        const std::string versionString(appName + " " UNPLAYER_VERSION);

        cxxopts::Options opts(appName, versionString);
        bool version = false;
        bool help = false;
        std::vector<std::string> files;
        opts.add_options()
            ("u,update-library", "update music library and exit", cxxopts::value<bool>(args.updateLibrary))
            ("r,reset-library", "reset music library and exit", cxxopts::value<bool>(args.resetLibrary))
            ("v,version", "display version information", cxxopts::value<bool>(version))
            ("h,help", "display this help", cxxopts::value<bool>(help))
            ("files", "", cxxopts::value<decltype(files)>(files));
            opts.parse_positional("files");
            opts.positional_help("files");
        try {
            const auto result(opts.parse(argc, argv));
            if (help) {
                std::cout << opts.help() << std::endl;
                args.exit = true;
                args.returnCode = EXIT_SUCCESS;
                return args;
            }
            if (version) {
                std::cout << versionString << std::endl;
                args.exit = true;
                args.returnCode = EXIT_SUCCESS;
                return args;
            }
            parseFiles(files, args);
        } catch (const cxxopts::OptionException& e) {
            std::cerr << e.what() << std::endl;
            args.exit = true;
            args.returnCode = EXIT_FAILURE;
        }

        return args;
    }
}
