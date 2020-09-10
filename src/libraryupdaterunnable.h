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

#ifndef UNPLAYER_LIBRARYUPDATERUNNABLE_H
#define UNPLAYER_LIBRARYUPDATERUNNABLE_H

#include <atomic>

#include <QObject>
#include <QRunnable>

#include "libraryutils.h"

namespace unplayer
{
    class LibraryUpdateRunnable final : public QObject, public QRunnable
    {
        Q_OBJECT
    public:
        void cancel();
        void run() override;

    private:
        std::atomic_bool mCancelFlag;

    signals:
        void stageChanged(unplayer::LibraryUtils::UpdateStage newStage);
        void foundFilesChanged(int found);
        void extractedFilesChanged(int extracted);
        void finished();
    };
}

#endif // UNPLAYER_LIBRARYUPDATERUNNABLE_H
