def options(context):
    context.load("compiler_cxx gnu_dirs qt5")


def configure(context):
    context.load("compiler_cxx gnu_dirs qt5")

    context.check_cfg(package="mpris-qt5", args="--libs --cflags")

    context.check_cfg(package="sailfishapp", args="--libs --cflags")
    context.env.LINKFLAGS_SAILFISHAPP = ["-pie", "-rdynamic"]

    context.env.INCLUDES_TAGLIB = ["{}/taglib/install/include/taglib".format(context.path.get_bld())]
    context.env.LIBPATH_TAGLIB = ["{}/taglib/install/lib".format(context.path.get_bld())]
    context.env.LIB_TAGLIB = ["tag"]
    context.env.RPATH_TAGLIB = "{}/harbour-unplayer/lib".format(context.env.DATADIR)


def build(context):
    context.program(
        target="harbour-unplayer",
        features="qt5",
        uselib=[
            "MPRIS-QT5",
            "QT5CONCURRENT",
            "QT5CORE",
            "QT5DBUS",
            "QT5GUI",
            "QT5MULTIMEDIA",
            "QT5QML",
            "QT5QUICK",
            "QT5SQL",
            "SAILFISHAPP",
            "TAGLIB"
        ],
        source=[
            "src/albumsmodel.cpp",
            "src/artistsmodel.cpp",
            "src/databasemodel.cpp",
            "src/directorycontentmodel.cpp",
            "src/directorycontentproxymodel.cpp",
            "src/directorytracksmodel.cpp",
            "src/filterproxymodel.cpp",
            "src/genresmodel.cpp",
            "src/librarydirectoriesmodel.cpp",
            "src/libraryutils.cpp",
            "src/main.cpp",
            "src/player.cpp",
            "src/playlistmodel.cpp",
            "src/playlistsmodel.cpp",
            "src/playlistutils.cpp",
            "src/queue.cpp",
            "src/queuemodel.cpp",
            "src/settings.cpp",
            "src/trackinfo.cpp",
            "src/tracksmodel.cpp",
            "src/utils.cpp"
        ],
        moc=[
            "src/albumsmodel.h",
            "src/artistsmodel.h",
            "src/directorycontentmodel.h",
            "src/directorycontentproxymodel.h",
            "src/directorytracksmodel.h",
            "src/filterproxymodel.h",
            "src/genresmodel.h",
            "src/librarydirectoriesmodel.h",
            "src/libraryutils.h",
            "src/player.h",
            "src/playlistmodel.h",
            "src/playlistsmodel.h",
            "src/playlistutils.h",
            "src/queue.h",
            "src/queuemodel.h",
            "src/settings.h",
            "src/trackinfo.h",
            "src/tracksmodel.h",
            "src/utils.h"
        ],
        cxxflags="-std=c++11",
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
