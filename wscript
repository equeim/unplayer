APP_VERSION = "1.2.3"


def options(context):
    context.load("compiler_cxx gnu_dirs qt5")

    context.add_option("--taglib-includepath", action="store")
    context.add_option("--taglib-libpath", action="store")

    context.add_option("--qtmpris-includepath", action="store")
    context.add_option("--qtmpris-libpath", action="store")
    context.add_option("--qtmpris-rpath-link", action="store")

    context.add_option("--harbour", action="store_true", default=False)


def configure(context):
    context.load("compiler_cxx gnu_dirs qt5")

    context.check_cfg(package="sailfishapp", args="--libs --cflags")
    context.env.LINKFLAGS_SAILFISHAPP = ["-pie", "-rdynamic"]

    context.env.INCLUDES_TAGLIB = [context.options.taglib_includepath]
    context.env.LIBPATH_TAGLIB = [context.options.taglib_libpath]
    context.env.LIB_TAGLIB = ["tag"]

    context.env.INCLUDES_QTMPRIS = [context.options.qtmpris_includepath]
    context.env.LIBPATH_QTMPRIS = [context.options.qtmpris_libpath]
    context.env.LIB_QTMPRIS = ["mpris-qt5"]
    context.env.LINKFLAGS_QTMPRIS = ["-Wl,-rpath-link={}".format(context.options.qtmpris_rpath_link)]

    context.env.HARBOUR = context.options.harbour


def build(context):
    context.program(
        target="harbour-unplayer",
        features="qt5",
        uselib=[
            "QT5CONCURRENT",
            "QT5CORE",
            "QT5DBUS",
            "QT5GUI",
            "QT5MULTIMEDIA",
            "QT5QML",
            "QT5QUICK",
            "QT5SQL",
            "QTMPRIS",
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
            "src/utils.cpp",
            "src/tagutils.cpp",
            "src/resources.qrc"
        ],
        moc=[
            "src/albumsmodel.h",
            "src/artistsmodel.h",
            "src/databasemodel.h",
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
        cxxflags=["-std=c++11", "-Wall", "-Wextra", "-pedantic"],
        defines=["QT_DEPRECATED_WARNINGS",
                 "QT_DISABLE_DEPRECATED_BEFORE=0x050200",
                 "UNPLAYER_VERSION=\"{}\"".format(APP_VERSION)],
        rpath="{}/harbour-unplayer/lib".format(context.env.DATADIR),
        lang=context.path.ant_glob("translations/*.ts")
    )

    context.install_files("${DATADIR}/harbour-unplayer/qml", "qml/main.qml")

    context.install_files("${DATADIR}/harbour-unplayer/qml/components",
                          context.path.ant_glob("qml/components/*.qml"))

    if context.env.HARBOUR:
        context.install_as("${DATADIR}/harbour-unplayer/qml/components/MediaKeys.qml", "qml/MediaKeysEmpty.qml")
    else:
        context.install_as("${DATADIR}/harbour-unplayer/qml/components/MediaKeys.qml", "qml/MediaKeysPrivate.qml")

    context.install_files("${DATADIR}/harbour-unplayer/translations",
                          context.path.get_bld().ant_glob("translations/*.qm",
                          quiet=True))

    context.install_files("${DATADIR}",
                          context.path.ant_glob("icons/**/*.png"),
                          relative_trick=True)

    context.install_files("${DATADIR}/applications",
                          "harbour-unplayer.desktop")
