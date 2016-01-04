#!/bin/sh

QT_SELECT=5

lupdate -pluralonly ../src ../qml -ts harbour-unplayer-en.ts
lupdate ../src ../qml -ts harbour-unplayer-de.ts \
                          harbour-unplayer-es.ts \
                          harbour-unplayer-fr.ts \
                          harbour-unplayer-it.ts \
                          harbour-unplayer-nb.ts \
                          harbour-unplayer-nl.ts \
                          harbour-unplayer-ru.ts \
                          harbour-unplayer-sv.ts
