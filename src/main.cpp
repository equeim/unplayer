/*
 * Unplayer
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

#include <memory>

#include <QGuiApplication>
#include <QQuickView>
#include <QQmlEngine>
#include <qqml.h>

#include <sailfishapp.h>

#include "libraryutils.h"
#include "settings.h"
#include "utils.h"

int main(int argc, char* argv[])
{
    using namespace unplayer;

    std::unique_ptr<QGuiApplication> app(SailfishApp::application(argc, argv));

    std::unique_ptr<QQuickView> view(SailfishApp::createView());

    Settings::instance();
    LibraryUtils::instance();

    Utils::registerTypes();

    view->setSource(SailfishApp::pathTo(QLatin1String("qml/main.qml")));
    view->show();

    return app->exec();
}
