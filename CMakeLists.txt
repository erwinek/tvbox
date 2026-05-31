cmake_minimum_required(VERSION 3.16)
project(tvbox_gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

set(TVBOX_SOURCES
    src/main.cpp
    src/app/App.cpp
    src/core/InputQueue.cpp
    src/core/StateMachine.cpp
    src/io/SerialReader.cpp
    src/io/Protocol.cpp
    src/io/KeyboardInput.cpp
    src/io/SerialInput.cpp
    src/ui/Renderer.cpp
    src/ui/Layout.cpp
    src/ui/FontManager.cpp
    src/ui/TextureCache.cpp
    src/ui/FrameSequencePlayer.cpp
    src/ui/widgets/Header.cpp
    src/ui/widgets/ScrollBar.cpp
    src/ui/widgets/LeaderboardWidget.cpp
    src/ui/widgets/Hud.cpp
    src/screens/AttractScreen.cpp
    src/screens/ModeSelectScreen.cpp
    src/screens/MeasureScreen.cpp
    src/screens/EndGameScreen.cpp
    src/game/ScoreEngine.cpp
    src/game/GameSession.cpp
    src/config/Config.cpp
    src/util/Logger.cpp
    src/store/LeaderboardStore.cpp
    src/media/VideoCapture.cpp
    src/media/AudioPlayer.cpp
    src/sync/SyncClient.cpp
)

add_executable(tvbox_gui ${TVBOX_SOURCES})
target_include_directories(tvbox_gui PRIVATE src)

if(WIN32 AND NOT MINGW)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(SDL2_ttf CONFIG REQUIRED)
    find_package(SDL2_image CONFIG REQUIRED)
    find_package(SDL2_mixer CONFIG REQUIRED)
    find_package(unofficial-sqlite3 CONFIG REQUIRED)
    find_package(CURL CONFIG REQUIRED)

    target_link_libraries(tvbox_gui PRIVATE
        Threads::Threads
        SDL2::SDL2
        SDL2::SDL2main
        SDL2_ttf::SDL2_ttf
        SDL2_image::SDL2_image
        SDL2_mixer::SDL2_mixer
        unofficial::sqlite3::sqlite3
        CURL::libcurl
    )
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
    pkg_check_modules(SQLITE3 REQUIRED sqlite3)
    pkg_check_modules(CURL REQUIRED libcurl)

    target_include_directories(tvbox_gui PRIVATE
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
        ${SDL2_IMAGE_INCLUDE_DIRS}
        ${SDL2_MIXER_INCLUDE_DIRS}
        ${SQLITE3_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
    )

    target_link_libraries(tvbox_gui PRIVATE
        Threads::Threads
        ${SDL2_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
        ${SDL2_IMAGE_LIBRARIES}
        ${SDL2_MIXER_LIBRARIES}
        ${SQLITE3_LIBRARIES}
        ${CURL_LIBRARIES}
    )
endif()

target_compile_definitions(tvbox_gui PRIVATE SDL_MAIN_HANDLED)

if(UNIX AND NOT APPLE)
    set(INSTALL_DIR "/home/erwinek/tvbox")

    install(TARGETS tvbox_gui DESTINATION ${INSTALL_DIR}/bin)
    install(DIRECTORY config/ DESTINATION ${INSTALL_DIR}/config)
    install(FILES scripts/tvbox.service DESTINATION /etc/systemd/system)

    add_custom_target(rpi5
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target tvbox_gui
        COMMAND ${CMAKE_COMMAND} -E make_directory ${INSTALL_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E make_directory ${INSTALL_DIR}/config
        COMMAND ${CMAKE_COMMAND} -E make_directory ${INSTALL_DIR}/data/videos
        COMMAND ${CMAKE_COMMAND} -E make_directory ${INSTALL_DIR}/assets/fonts
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:tvbox_gui> ${INSTALL_DIR}/bin/tvbox_gui
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/config/app.yaml ${INSTALL_DIR}/config/app.yaml
        COMMAND sudo cp ${CMAKE_SOURCE_DIR}/scripts/tvbox.service /etc/systemd/system/tvbox.service
        COMMAND sudo systemctl daemon-reload
        COMMAND sudo systemctl enable tvbox.service
        COMMENT "Deploy tvbox to RPI5 at ${INSTALL_DIR}"
        VERBATIM
    )
endif()
