/*
    SDL3 menu and overlay drawing.

    State transitions live in main_sdl3.cpp; these functions only render the
    current screen from data passed in by the caller.
*/
#pragma once

#include "sdl3_menu.h"
#include "../persistence/leaderboard.h"
#include "../persistence/stats.h"
#include "../persistence/achievements.h"
#include "../persistence/save_state.h"
#include "../persistence/level_file.h"
#include "../game/game.h"

#include <SDL3/SDL.h>
#include <cstdint>
#include <string>
#include <vector>

namespace si {

enum class Screen {
    USERNAME_INPUT,
    MAIN_MENU,
    COOP_MENU,
    COOP_JOIN_INPUT,
    COOP_CONNECTING,
    COOP_ERROR,
    SETTINGS,
    LEADERBOARD,
    STATS_ACHIEVEMENTS,
    DIFFICULTY_SELECT,
    CUSTOM_LEVELS,
    LEVEL_PREVIEW,
    LEVEL_EDITOR,
    LEVEL_EDITOR_TEXT_INPUT,
    LEVEL_LOAD_ERROR,
    REPLAY_BROWSER,
    REPLAY_INPUT,
    REPLAY_SUMMARY,
    PLAYING,
    PAUSED,
    GAME_OVER,
    QUIT
};

struct ReplaySummaryView {
    std::string file;
    std::string difficulty;
    std::string mode;
    std::string player;
    std::string status;
    std::uint32_t seed = 0;
    int expectedScore = -1;
    int actualScore = 0;
    int expectedLevel = -1;
    int actualLevel = 0;
};

constexpr int SCR_W = 1120;
constexpr int SCR_H = 512;

void draw_dimmed_backdrop(SDL_Renderer* ren, std::uint8_t alpha);

void draw_menu_backdrop(SDL_Renderer* ren);

void draw_screen_title(SDL_Renderer* ren, const std::string& title,
                        float topY = 40.0f);

void draw_footer(SDL_Renderer* ren, const std::string& hint);

void draw_username_input(SDL_Renderer* ren,
                          const std::string& buffer,
                          bool cursorBlinkOn);

void draw_main_menu(SDL_Renderer* ren,
                     const MenuList& menu,
                     const std::string& user,
                     bool hasSave);

void draw_settings(SDL_Renderer* ren, const MenuList& menu);

void draw_coop_menu(SDL_Renderer* ren,
                    const MenuList& menu,
                    int port);

void draw_coop_join_input(SDL_Renderer* ren,
                          const std::string& buffer,
                          const std::string& error,
                          bool cursorBlinkOn);

void draw_coop_connecting(SDL_Renderer* ren,
                          const std::string& status);

void draw_coop_error(SDL_Renderer* ren,
                     const std::string& error);

void draw_difficulty_select(SDL_Renderer* ren, const MenuList& menu);

void draw_custom_levels(SDL_Renderer* ren,
                        const MenuList& menu,
                        const std::string& error,
                        bool hasLevelFiles);

void draw_level_preview(SDL_Renderer* ren,
                        const LevelFile& level,
                        const std::string& path,
                        const MenuList& menu);

void draw_level_editor(SDL_Renderer* ren,
                       const LevelFile& level,
                       const std::string& path,
                       int activeGrid,
                       int alienRow,
                       int alienCol,
                       int shieldRow,
                       int shieldCol,
                       const std::string& message);

void draw_level_editor_text_input(SDL_Renderer* ren,
                                  const std::string& label,
                                  const std::string& buffer,
                                  const std::string& error,
                                  bool cursorBlinkOn);

void draw_level_load_error(SDL_Renderer* ren,
                           const std::string& path,
                           const std::string& error);

void draw_replay_browser(SDL_Renderer* ren,
                         const MenuList& menu,
                         const std::string& error,
                         bool hasReplayFiles);

void draw_replay_input(SDL_Renderer* ren,
                        const std::string& buffer,
                        const std::string& error,
                        bool cursorBlinkOn);

void draw_replay_summary(SDL_Renderer* ren,
                          const ReplaySummaryView& summary,
                          const MenuList& menu);

void draw_leaderboard(SDL_Renderer* ren,
                       const std::vector<Record>& lb,
                       const std::string& currentUser);

void draw_stats_achievements(SDL_Renderer* ren,
                              const std::string& user,
                              const Stats& stats,
                              const std::vector<Achievement>& ach);

void draw_pause_overlay(SDL_Renderer* ren, const MenuList& menu,
                         const Game& g);

void draw_game_over(SDL_Renderer* ren, const MenuList& menu,
                     const Game& g, bool isNewBest);

}
