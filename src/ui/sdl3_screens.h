// sdl3_screens.h - menu and overlay screens for the SDL3 build.
//
// Each free function in this header draws ONE screen. State machine
// dispatch and event handling live in main_sdl3.cpp - these are pure
// rendering routines plus thin helpers.
//
// Why functions and not classes: each screen is essentially stateless
// at render time - the state it cares about lives in main_sdl3.cpp's
// AppState struct. Functions are simpler than wrapping each screen
// in its own class.
#pragma once

#include "sdl3_menu.h"
#include "../persistence/leaderboard.h"
#include "../persistence/stats.h"
#include "../persistence/achievements.h"
#include "../persistence/save_state.h"
#include "../game/game.h"

#include <SDL3/SDL.h>
#include <string>
#include <vector>

namespace si {

// Screen identifiers used by the SDL3 state machine.
enum class Screen {
    USERNAME_INPUT,
    MAIN_MENU,
    SETTINGS,
    LEADERBOARD,
    STATS_ACHIEVEMENTS,
    DIFFICULTY_SELECT,
    REPLAY_INPUT,
    PLAYING,        // game is running; main loop owns rendering
    PAUSED,
    GAME_OVER,
    QUIT
};

// Shared drawing helpers.

// Window dimensions are taken from the renderer; we hard-code them
// here for simplicity (same constants).
constexpr int SCR_W = 1120;
constexpr int SCR_H = 512;

// Draw a dimmed full-window backdrop (used by pause overlay and
// menus over a paused game).
void draw_dimmed_backdrop(SDL_Renderer* ren, std::uint8_t alpha);

// Draw a solid menu backdrop (deep blue, like the gameplay HUD).
void draw_menu_backdrop(SDL_Renderer* ren);

// Centered title at the top of a menu screen, big text.
void draw_screen_title(SDL_Renderer* ren, const std::string& title,
                        float topY = 40.0f);

// Footer hint, small dim text at the bottom.
void draw_footer(SDL_Renderer* ren, const std::string& hint);

// Specific screens and overlays.

// USERNAME_INPUT
// Caller passes the current entry buffer and whether text input is
// active. The screen just draws; the caller handles SDL_EVENT_TEXT_INPUT
// to mutate the buffer.
void draw_username_input(SDL_Renderer* ren,
                          const std::string& buffer,
                          bool cursorBlinkOn);

// MAIN_MENU
void draw_main_menu(SDL_Renderer* ren,
                     const MenuList& menu,
                     const std::string& user,
                     bool hasSave);

// SETTINGS
void draw_settings(SDL_Renderer* ren, const MenuList& menu);

// DIFFICULTY_SELECT
void draw_difficulty_select(SDL_Renderer* ren, const MenuList& menu);

// REPLAY_INPUT
void draw_replay_input(SDL_Renderer* ren,
                        const std::string& buffer,
                        const std::string& error,
                        bool cursorBlinkOn);

// LEADERBOARD - shows top 10
void draw_leaderboard(SDL_Renderer* ren,
                       const std::vector<Record>& lb,
                       const std::string& currentUser);

// STATS + ACHIEVEMENTS for the current user
void draw_stats_achievements(SDL_Renderer* ren,
                              const std::string& user,
                              const Stats& stats,
                              const std::vector<Achievement>& ach);

// PAUSED overlay - drawn over the live game.
void draw_pause_overlay(SDL_Renderer* ren, const MenuList& menu,
                         const Game& g);

// GAME_OVER screen - drawn over the final game frame.
// `isNewBest` highlights the entry if the player just beat their best.
void draw_game_over(SDL_Renderer* ren, const MenuList& menu,
                     const Game& g, bool isNewBest);

} // namespace si
