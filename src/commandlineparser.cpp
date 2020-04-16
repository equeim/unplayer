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

#include "clara.hpp"

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

                            args.files.push_back(info.absoluteFilePath());
                        }
                    } else {
                        const QUrl url(file);
                        if (url.isLocalFile()) {
                            if (QFileInfo(url.path()).isFile()) {
                                args.files.push_back(url.path());
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

        using namespace clara;

        bool version = false;
        bool help = false;
        std::vector<std::string> files;
        auto cli = Opt(args.updateLibrary)["-u"]["--update-library"]("update music library and exit") |
                   Opt(args.resetLibrary)["-r"]["--reset-library"]("reset music library and exit") |
                   Opt(version)["-v"]["--version"]("display version information") |
                   Help(help) |
                   Arg(files, "files");
        auto result = cli.parse(Args(argc, argv));
        if (!result) {
            std::cerr << result.errorMessage() << '\n';
            args.exit = true;
            args.returnCode = 1;
        } else if (help) {
            std::cout << cli << '\n';
            args.exit = true;
        } else if (version) {
            std::cout << cli.m_exeName.name() << " " UNPLAYER_VERSION "\n";
            args.exit = true;
        } else {
            parseFiles(files, args);
        }

        return args;
    }
}
