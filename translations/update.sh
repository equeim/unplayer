#!/bin/sh

QT_SELECT=5

lupdate -pluralonly ../src ../qml -ts harbour-unplayer-en.ts
lupdate ../src ../qml -ts harbour-unplayer-ru.ts harbour-unplayer-sv.ts
