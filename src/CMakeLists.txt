set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Core 5.6 REQUIRED CONFIG)
find_package(Qt5Concurrent CONFIG REQUIRED)
find_package(Qt5DBus CONFIG REQUIRED)
find_package(Qt5Multimedia)
find_package(Qt5Quick CONFIG REQUIRED)
find_package(Qt5Sql CONFIG REQUIRED)

find_package(PkgConfig REQUIRED)

pkg_check_modules(SAILFISHAPP REQUIRED sailfishapp)

pkg_check_modules(QTMPRIS REQUIRED mpris-qt5)
if (QTMPRIS_STATIC)
    set(qtmpris_ldflags ${QTMPRIS_STATIC_LDFLAGS})
else()
    set(qtmpris_ldflags ${QTMPRIS_LDFLAGS})
endif()

pkg_check_modules(TAGLIB REQUIRED taglib)
if (TAGLIB_STATIC)
    set(taglib_ldflags ${TAGLIB_STATIC_LDFLAGS})
else()
    set(taglib_ldflags ${TAGLIB_LDFLAGS})
endif()

add_executable("${PROJECT_NAME}"
    abstractlibrarymodel.cpp
    albumsmodel.cpp
    artistsmodel.cpp
    asyncloadingmodel.h
    commandlineparser.cpp
    directorycontentmodel.cpp
    directorycontentproxymodel.cpp
    directorytracksmodel.cpp
    fileutils.cpp
    filterproxymodel.cpp
    genresmodel.cpp
    librarydirectoriesmodel.cpp
    librarymigrator.cpp
    librarytracksadder.cpp
    libraryupdaterunnable.cpp
    libraryutils.cpp
    main.cpp
    mediaartutils.cpp
    player.cpp
    playlistmodel.cpp
    playlistsmodel.cpp
    playlistutils.cpp
    queue.cpp
    queuemodel.cpp
    settings.cpp
    signalhandler.cpp
    trackinfo.cpp
    tracksmodel.cpp
    utils.cpp
    tagutils.cpp
    resources.qrc
)

set_target_properties("${PROJECT_NAME}" PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries("${PROJECT_NAME}"
    Qt5::Concurrent
    Qt5::DBus
    Qt5::Multimedia
    Qt5::Quick
    Qt5::Sql
    ${SAILFISHAPP_LDFLAGS}
    ${qtmpris_ldflags}
    ${taglib_ldflags}
)

target_include_directories("${PROJECT_NAME}" PRIVATE
    ${SAILFISHAPP_INCLUDE_DIRS}
    ${QTMPRIS_INCLUDE_DIRS}
    ${TAGLIB_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/3rdparty/cxxopts/include
)

target_compile_definitions("${PROJECT_NAME}" PRIVATE
    QT_DEPRECATED_WARNINGS
    QT_DISABLE_DEPRECATED_BEFORE=0x050600
    QT_MESSAGELOGCONTEXT
    UNPLAYER_VERSION="${PROJECT_VERSION}"
)

target_compile_options("${PROJECT_NAME}" PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wnon-virtual-dtor
    -Wcast-align
    -Woverloaded-virtual
    -Wconversion
    -Wsign-conversion
    -Wlogical-op
    -Wdouble-promotion
    -Wformat=2
    -Werror=format
    ${SAILFISHAPP_CFLAGS_OTHER}
    ${QTMPRIS_CFLAGS_OTHER}
    ${TAGLIB_CFLAGS_OTHER}
)

install(TARGETS "${PROJECT_NAME}" DESTINATION "${CMAKE_INSTALL_BINDIR}")
