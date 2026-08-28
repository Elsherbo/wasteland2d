cmake_minimum_required(VERSION 3.20)
project(wasteland2d CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable testing support
enable_testing()

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
target_link_libraries(engine PUBLIC SDL2::SDL2 SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf tinyxml2::tinyxml2 glm::glm box2d::box2d nlohmann_json::nlohmann_json)

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

# --- tests (standalone executables, no assets needed) -----------------
# Test helper: cross-platform temporary directory
if(WIN32)
    # Windows uses TEMP environment variable
    if(DEFINED ENV{TEMP})
        # Convert backslashes to forward slashes for C preprocessor
        string(REPLACE "\\" "/" TEST_TMP_DIR "$ENV{TEMP}")
    else()
        set(TEST_TMP_DIR "C:/Temp")
    endif()
else()
    set(TEST_TMP_DIR "/tmp")
endif()

# Helper function to add a test
function(add_wasteland_test test_name source_file needs_game_headers)
    add_executable(${test_name} ${source_file})
    target_include_directories(${test_name} PRIVATE
        "${CMAKE_SOURCE_DIR}/engine"
    )
    if(${needs_game_headers})
        target_include_directories(${test_name} PRIVATE
            "${CMAKE_SOURCE_DIR}/game"
        )
    endif()
    target_compile_definitions(${test_name} PRIVATE TEST_TMP_DIR="${TEST_TMP_DIR}")
    
    # Add CTest test - run from project root so relative paths work
    add_test(NAME ${test_name} COMMAND ${test_name} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endfunction()

# Headless tests (no external dependencies)
add_wasteland_test(attachment_test tests/attachment_test.cpp FALSE)
target_link_libraries(attachment_test PRIVATE engine)

# JSON-dependent tests (need nlohmann_json)
add_wasteland_test(inventory_test tests/inventory_test.cpp TRUE)
target_link_libraries(inventory_test PRIVATE engine nlohmann_json::nlohmann_json)
target_sources(inventory_test PRIVATE game/data/ItemDatabase.cpp)

add_wasteland_test(equipment_test tests/equipment_test.cpp TRUE)
target_link_libraries(equipment_test PRIVATE engine nlohmann_json::nlohmann_json)
target_sources(equipment_test PRIVATE game/data/ItemDatabase.cpp)

add_wasteland_test(encumbrance_test tests/encumbrance_test.cpp TRUE)
target_link_libraries(encumbrance_test PRIVATE engine nlohmann_json::nlohmann_json)
target_sources(encumbrance_test PRIVATE game/data/ItemDatabase.cpp)

add_wasteland_test(dragdrop_test tests/dragdrop_test.cpp TRUE)
target_link_libraries(dragdrop_test PRIVATE engine nlohmann_json::nlohmann_json)
target_sources(dragdrop_test PRIVATE game/data/ItemDatabase.cpp)

add_wasteland_test(useitem_test tests/useitem_test.cpp TRUE)
target_link_libraries(useitem_test PRIVATE engine nlohmann_json::nlohmann_json)
target_sources(useitem_test PRIVATE game/data/ItemDatabase.cpp)

# Physics-dependent tests (need Box2D)
add_wasteland_test(combat_test tests/combat_test.cpp TRUE)
target_link_libraries(combat_test PRIVATE engine nlohmann_json::nlohmann_json box2d::box2d glm::glm)
target_sources(combat_test PRIVATE game/data/ItemDatabase.cpp)

add_wasteland_test(melee_test tests/melee_test.cpp TRUE)
target_link_libraries(melee_test PRIVATE engine nlohmann_json::nlohmann_json box2d::box2d glm::glm)
target_sources(melee_test PRIVATE game/data/ItemDatabase.cpp)

add_wasteland_test(kill_scenario_test tests/kill_scenario_test.cpp TRUE)
target_link_libraries(kill_scenario_test PRIVATE engine nlohmann_json::nlohmann_json box2d::box2d glm::glm)
target_sources(kill_scenario_test PRIVATE game/data/ItemDatabase.cpp)

add_wasteland_test(registry_reference_safety_test tests/registry_reference_safety_test.cpp FALSE)
target_link_libraries(registry_reference_safety_test PRIVATE engine)

add_wasteland_test(eventbus_test tests/eventbus_test.cpp TRUE)
target_link_libraries(eventbus_test PRIVATE engine)

add_wasteland_test(servicelocator_test tests/servicelocator_test.cpp TRUE)
target_link_libraries(servicelocator_test PRIVATE engine)

# SDL-dependent tests
add_wasteland_test(font_runtime_test tests/font_runtime_test.cpp FALSE)
target_link_libraries(font_runtime_test PRIVATE engine SDL2::SDL2 SDL2_ttf::SDL2_ttf)
