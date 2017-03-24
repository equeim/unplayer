def options(context):
    context.load("compiler_cxx gnu_dirs qt5")


def configure(context):
    context.load("compiler_cxx gnu_dirs qt5")

    context.check_cfg(package="mpris-qt5", args="--libs --cflags")
    context.check_cfg(package="Qt5Sparql", args="--libs --cflags")
    context.check_cfg(package="sailfishapp", args="--libs --cflags")


def build(context):
    context.program(
        target="harbour-unplayer",
        features="qt5",
        uselib=[
            "MPRIS-QT5",
            "QT5CORE",
            "QT5DBUS",
            "QT5GUI",
            "QT5MULTIMEDIA",
            "QT5QML",
            "QT5QUICK",
            "QT5SPARQL",
            "SAILFISHAPP"
        ],
        source=[
            "src/directorytracksmodel.cpp",
            "src/filepickermodel.cpp",
            "src/filterproxymodel.cpp",
            "src/main.cpp",
            "src/player.cpp",
            "src/playlistmodel.cpp",
            "src/playlistutils.cpp",
            "src/queue.cpp",
            "src/queuemodel.cpp",
            "src/utils.cpp"
        ],
        moc=[
            "src/directorytracksmodel.h",
            "src/filepickermodel.h",
            "src/filterproxymodel.h",
            "src/player.h",
            "src/playlistmodel.h",
            "src/playlistutils.h",
            "src/queue.h",
            "src/queuemodel.h",
            "src/utils.h"
        ],
        cxxflags="-std=c++11",
        linkflags="-pie -rdynamic",
        lang=[
            "translations/harbour-unplayer-en",
            "translations/harbour-unplayer-es",
            "translations/harbour-unplayer-fr",
            "translations/harbour-unplayer-it",
            "translations/harbour-unplayer-nb",
            "translations/harbour-unplayer-nl",
            "translations/harbour-unplayer-ru",
            "translations/harbour-unplayer-sv"
        ]
    )

    context.install_files("${DATADIR}/harbour-unplayer",
                          context.path.ant_glob("qml/**/*.qml"),
                          relative_trick=True)

    context.install_files("${DATADIR}/harbour-unplayer/translations",
                          context.path.get_bld().ant_glob("translations/*.qm"))

    context.install_files("${DATADIR}",
                          context.path.ant_glob("icons/**/*.png"),
                          relative_trick=True)

    context.install_files("${DATADIR}/applications",
                          "harbour-unplayer.desktop")
