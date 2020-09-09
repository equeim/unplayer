#!/bin/sh

if command -v lupdate-qt5 > /dev/null 2>&1; then
    _lupdate=lupdate-qt5
else
    _lupdate=lupdate
fi

_dir="$(realpath $(dirname $0))"
cd "$_dir"

QT_SELECT=5 $_lupdate ../src ../qml -ts harbour-unplayer-en.ts \
                                        harbour-unplayer-ar.ts \
                                        harbour-unplayer-de.ts \
                                        harbour-unplayer-es.ts \
                                        harbour-unplayer-fr.ts \
                                        harbour-unplayer-hr.ts \
                                        harbour-unplayer-it.ts \
                                        harbour-unplayer-nb.ts \
                                        harbour-unplayer-nl.ts \
                                        harbour-unplayer-nl_BE.ts \
                                        harbour-unplayer-pl.ts \
                                        harbour-unplayer-ru.ts \
                                        harbour-unplayer-sv.ts \
                                        harbour-unplayer-zh_CN.ts
