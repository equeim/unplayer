cmake_minimum_required(VERSION 3.1.0)
cmake_policy(SET CMP0071 NEW)

project(harbour-unplayer VERSION 2.0.3 LANGUAGES CXX)

include(GNUInstallDirs)

option(HARBOUR "Build for Harbour" ON)
option(QTMPRIS_STATIC "Link with qtmpris statically" OFF)
option(TAGLIB_STATIC "Link with taglib statically" OFF)

add_subdirectory("src")
add_subdirectory("translations")

install(DIRECTORY "icons/hicolor" DESTINATION "${CMAKE_INSTALL_DATADIR}/icons")

set(qml_dir "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/qml")
install(FILES "qml/main.qml" DESTINATION "${qml_dir}")
install(DIRECTORY "qml/components" DESTINATION "${qml_dir}")
if (HARBOUR)
    set(media_keys "qml/MediaKeysEmpty.qml")
else()
    set(media_keys "qml/MediaKeysPrivate.qml")
endif()
install(FILES "${media_keys}"
        DESTINATION "${qml_dir}/components"
        RENAME "MediaKeys.qml")

install(FILES "${PROJECT_NAME}.desktop" DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
