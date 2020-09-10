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

#ifndef UNPLAYER_SIGNALHANDLER_H
#define UNPLAYER_SIGNALHANDLER_H

#include <atomic>

namespace unplayer
{
    class SignalHandler
    {
    public:
        static std::atomic_bool exitRequested;

        static void setupHandlers();
        static void setupNotifier();

        SignalHandler(const SignalHandler&) = delete;
        SignalHandler(SignalHandler&&) = delete;
        SignalHandler& operator=(const SignalHandler&) = delete;
        SignalHandler& operator=(SignalHandler&&) = delete;
    };
}

#endif // UNPLAYER_SIGNALHANDLER_H
