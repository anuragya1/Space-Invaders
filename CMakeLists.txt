cmake_minimum_required(VERSION 3.15)
project(si_pro LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(SI_CORE_SOURCES
    src/core/rng.cpp
    src/core/difficulty.cpp
    src/config/config.cpp
    src/config/cli.cpp
    src/debug/logger.cpp
    src/render/rbuf.cpp
    src/persistence/save_state.cpp
    src/persistence/leaderboard.cpp
    src/persistence/stats.cpp
    src/persistence/achievements.cpp
    src/persistence/replay_file.cpp
    src/persistence/level_file.cpp
    src/persistence/telemetry.cpp
    src/net/tcp_socket.cpp
    src/input/input_source.cpp
    src/input/keyboard_source.cpp
    src/input/ai_source.cpp
    src/input/replay_source.cpp
    src/input/coop_source.cpp
    src/game/game.cpp
    src/game/game_step.cpp
    src/game/game_render.cpp
    src/editor/level_editor.cpp
    src/ui/banner.cpp
    src/ui/menus.cpp
    src/ui/tools.cpp
    src/i18n/strings.cpp
)

if (WIN32)
    list(APPEND SI_CORE_SOURCES src/platform/platform_win32.cpp)
else()
    list(APPEND SI_CORE_SOURCES src/platform/platform_posix.cpp)
    find_package(Threads REQUIRED)
endif()

function(si_apply_common_target_settings target_name)
    target_include_directories(${target_name} PRIVATE src)
    if (WIN32)
        target_link_libraries(${target_name} PRIVATE ws2_32)
    else()
        target_link_libraries(${target_name} PRIVATE Threads::Threads)
    endif()
endfunction()

add_executable(si_pro src/main.cpp ${SI_CORE_SOURCES})
si_apply_common_target_settings(si_pro)

enable_testing()

set(SI_TESTS
    test_rng
    test_replay
    test_level
    test_config
    test_ai
    test_director
    test_events
    test_determinism
)

foreach (test_name IN LISTS SI_TESTS)
    add_executable(${test_name} tests/${test_name}.cpp ${SI_CORE_SOURCES})
    if (test_name STREQUAL "test_director")
        target_sources(${test_name} PRIVATE src/director/director.cpp)
    endif()
    si_apply_common_target_settings(${test_name})
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

option(SI_BUILD_SDL3 "Build the SDL3 windowed binary si_pro_sdl3" OFF)
if (SI_BUILD_SDL3)
    find_package(SDL3 CONFIG REQUIRED)
    add_executable(si_pro_sdl3
        src/main_sdl3.cpp
        src/render/sdl3_renderer.cpp
        src/render/sdl3_particles.cpp
        src/render/sdl3_sprites.cpp
        src/audio/sdl3_audio.cpp
        src/ui/sdl3_file_browser.cpp
        src/ui/sdl3_menu.cpp
        src/ui/sdl3_screens.cpp
        src/input/sdl3_keyboard.cpp
        src/director/director.cpp
        ${SI_CORE_SOURCES})
    target_link_libraries(si_pro_sdl3 PRIVATE SDL3::SDL3)
    si_apply_common_target_settings(si_pro_sdl3)
endif()
