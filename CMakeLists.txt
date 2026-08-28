cmake_minimum_required(VERSION 3.20)
project(wasteland2d CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra)
endif()

# --- SDL2 -------------------------------------------------------------
# Preferred path: vcpkg (find_package config mode). On Windows with MSVC,
# configure the project with:
#   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake
# after: vcpkg install sdl2 sdl2-image sdl2-mixer sdl2-ttf nlohmann-json tinyxml2
find_package(SDL2 CONFIG QUIET)
find_package(SDL2_image CONFIG QUIET)
find_package(SDL2_ttf CONFIG QUIET)
find_package(tinyxml2 CONFIG QUIET)
find_package(glm CONFIG QUIET)
find_package(box2d CONFIG QUIET)
# nlohmann_json ships a proper CMake config on both vcpkg and apt
# (nlohmann-json3-dev) — unlike SDL2/tinyxml2/box2d above, no
# pkg-config fallback dance is needed on Linux either.
find_package(nlohmann_json CONFIG REQUIRED)

if(NOT SDL2_FOUND)
    if(WIN32)
        message(FATAL_ERROR
            "SDL2 not found via vcpkg. Run: vcpkg install sdl2:x64-windows "
            "(and make sure -DCMAKE_TOOLCHAIN_FILE points at vcpkg.cmake).")
    endif()
    # Fallback for Linux/dev-container testing where SDL2 is installed
    # system-wide via apt (libsdl2-dev) rather than vcpkg.
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    add_library(SDL2::SDL2 ALIAS PkgConfig::SDL2)
endif()

if(NOT SDL2_image_FOUND)
    if(WIN32)
        message(FATAL_ERROR "SDL2_image not found via vcpkg. Run: vcpkg install sdl2-image:x64-windows")
    endif()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
    add_library(SDL2_image::SDL2_image ALIAS PkgConfig::SDL2_IMAGE)
endif()

if(NOT SDL2_ttf_FOUND)
    if(WIN32)
        message(FATAL_ERROR "SDL2_ttf not found via vcpkg. Run: vcpkg install sdl2-ttf:x64-windows")
    endif()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)
    add_library(SDL2_ttf::SDL2_ttf ALIAS PkgConfig::SDL2_TTF)
endif()

if(NOT tinyxml2_FOUND)
    if(WIN32)
        message(FATAL_ERROR "tinyxml2 not found via vcpkg. Run: vcpkg install tinyxml2:x64-windows")
    endif()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(TINYXML2 REQUIRED IMPORTED_TARGET tinyxml2)
    add_library(tinyxml2::tinyxml2 ALIAS PkgConfig::TINYXML2)
endif()

if(NOT glm_FOUND)
    if(WIN32)
        message(FATAL_ERROR "glm not found via vcpkg. Run: vcpkg install glm:x64-windows")
    endif()
    # GLM is header-only — a plain INTERFACE target is enough; no
    # pkg-config module exists for it on most distros, so this just
    # relies on it being on the system include path (apt: libglm-dev).
    add_library(glm INTERFACE)
    add_library(glm::glm ALIAS glm)
endif()

if(NOT box2d_FOUND)
    if(WIN32)
        message(FATAL_ERROR "box2d not found via vcpkg. Run: vcpkg install box2d:x64-windows")
    endif()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(BOX2D REQUIRED IMPORTED_TARGET box2d)
    add_library(box2d::box2d ALIAS PkgConfig::BOX2D)
endif()

# --- engine (framework layer, static lib) ------------------------------
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/engine/*.cpp"
)
add_library(engine STATIC ${ENGINE_SOURCES})
target_include_directories(engine PUBLIC
    "${CMAKE_SOURCE_DIR}/engine"
)
target_link_libraries(engine PUBLIC SDL2::SDL2 SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf tinyxml2::tinyxml2 glm::glm box2d::box2d)

# --- game (this specific game, executable) -----------------------------
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/game/*.cpp"
)
add_executable(wasteland2d ${GAME_SOURCES})
target_include_directories(wasteland2d PRIVATE
    "${CMAKE_SOURCE_DIR}/game"
)
target_link_libraries(wasteland2d PRIVATE engine nlohmann_json::nlohmann_json)

# Copy assets next to the built binary so relative paths ("assets/...")
# resolve the same in-editor and when double-clicking the exe.
add_custom_command(TARGET wasteland2d POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:wasteland2d>/assets"
)
