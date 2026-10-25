cmake_minimum_required(VERSION 3.16)
project(SonicExeLastProtocol LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SFML 3.x. If CMake can't find it automatically:
#   cmake -B build -DSFML_DIR=/path/to/SFML/lib/cmake/SFML
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

# ── Main game ──────────────────────────────────────────────
# Everything in src/ except nothing - extras/main_demo.cpp lives outside
# this glob specifically so it can never collide with src/main.cpp's
# main() at link time (both define one).
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(SonicExeLastProtocol ${SOURCES})

target_include_directories(SonicExeLastProtocol PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(SonicExeLastProtocol PRIVATE SFML::Graphics SFML::Window SFML::System)

if (MSVC)
    # Several UTF-8 glyphs (✦ ▸ ◂ — Ⅰ Ⅱ etc.) are embedded directly in the
    # source for HUD/title text; MSVC needs this flag to interpret both the
    # source file and \u escape sequences as UTF-8 rather than the system
    # codepage. GCC/Clang already default to UTF-8 and don't need it.
    target_compile_options(SonicExeLastProtocol PRIVATE /utf-8)
endif()

add_custom_command(TARGET SonicExeLastProtocol POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:SonicExeLastProtocol>/assets"
)

# ── Optional: standalone control-system demo ────────────────
# extras/main_demo.cpp exercises MetalSonicController + CorruptionDirector
# in isolation (no boss, no level geometry, no full Game) - handy for
# testing physics/curse/animation changes without booting the whole game.
# Off by default. Enable with: cmake -B build -DBUILD_CONTROL_DEMO=ON
option(BUILD_CONTROL_DEMO "Build the standalone Metal Sonic control-system demo" OFF)
if (BUILD_CONTROL_DEMO)
    set(DEMO_SOURCES
        "${CMAKE_SOURCE_DIR}/extras/main_demo.cpp"
        "${CMAKE_SOURCE_DIR}/src/Input.cpp"
        "${CMAKE_SOURCE_DIR}/src/Clock.cpp"
        "${CMAKE_SOURCE_DIR}/src/SpriteAnimation.cpp"
        "${CMAKE_SOURCE_DIR}/src/PhysicsBody.cpp"
        "${CMAKE_SOURCE_DIR}/src/MetalSonicController.cpp"
        "${CMAKE_SOURCE_DIR}/src/CorruptionDirector.cpp"
    )
    add_executable(MetalSonicControlDemo ${DEMO_SOURCES})
    target_include_directories(MetalSonicControlDemo PRIVATE "${CMAKE_SOURCE_DIR}/include")
    target_link_libraries(MetalSonicControlDemo PRIVATE SFML::Graphics SFML::Window SFML::System)
    if (MSVC)
        target_compile_options(MetalSonicControlDemo PRIVATE /utf-8)
    endif()
    add_custom_command(TARGET MetalSonicControlDemo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:MetalSonicControlDemo>/assets"
    )
endif()
