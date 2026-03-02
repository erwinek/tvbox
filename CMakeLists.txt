cmake_minimum_required(VERSION 3.16)
project(tvbox_gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED sdl2_ttf)
pkg_check_modules(SDL2_IMAGE REQUIRED sdl2_image)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(CURL REQUIRED libcurl)

add_executable(tvbox_gui
    src/main.cpp
    src/app/App.cpp
    src/ui/UiRenderer.cpp
    src/io/SerialReader.cpp
    src/config/Config.cpp
    src/util/Logger.cpp
    src/store/LeaderboardStore.cpp
    src/media/VideoCapture.cpp
    src/sync/SyncClient.cpp
)

target_include_directories(tvbox_gui PRIVATE
    src
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(tvbox_gui PRIVATE
    Threads::Threads
    ${SDL2_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${CURL_LIBRARIES}
)

target_compile_definitions(tvbox_gui PRIVATE
    SDL_MAIN_HANDLED
)
