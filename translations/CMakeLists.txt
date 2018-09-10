find_package(Qt5LinguistTools CONFIG REQUIRED)

qt5_add_translation(qm_files
    harbour-unplayer-ar.ts
    harbour-unplayer-de.ts
    harbour-unplayer-en.ts
    harbour-unplayer-es.ts
    harbour-unplayer-fr.ts
    harbour-unplayer-it.ts
    harbour-unplayer-nb.ts
    harbour-unplayer-nl_BE.ts
    harbour-unplayer-nl.ts
    harbour-unplayer-ru.ts
    harbour-unplayer-sv.ts
)

add_custom_target(translations ALL DEPENDS ${qm_files})

install(FILES ${qm_files} DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/translations")
