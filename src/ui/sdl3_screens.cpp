// sdl3_screens.cpp - draws each menu screen and overlay.
#include "sdl3_screens.h"

#include "../core/difficulty.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>

namespace si {

namespace {

// Local text helpers - duplicated from sdl3_menu.cpp on purpose so
// these screens don't pull in the renderer's full header.
void render_text(SDL_Renderer* ren, const std::string& msg,
                 float x, float y, float scale) {
    if (scale == 1.0f) {
        SDL_RenderDebugText(ren, x, y, msg.c_str());
        return;
    }
    float pSX = 1.0f, pSY = 1.0f;
    SDL_GetRenderScale(ren, &pSX, &pSY);
    SDL_SetRenderScale(ren, scale, scale);
    SDL_RenderDebugText(ren, x / scale, y / scale, msg.c_str());
    SDL_SetRenderScale(ren, pSX, pSY);
}

void render_text_centered(SDL_Renderer* ren, const std::string& msg,
                          float cx, float y, float scale) {
    const float w = static_cast<float>(msg.size()) * 8.0f * scale;
    render_text(ren, msg, cx - w * 0.5f, y, scale);
}

void fill_rect(SDL_Renderer* ren, float x, float y, float w, float h) {
    SDL_FRect r{ x, y, w, h };
    SDL_RenderFillRect(ren, &r);
}

void set_col(SDL_Renderer* ren, std::uint8_t r, std::uint8_t g,
             std::uint8_t b, std::uint8_t a = 255) {
    SDL_SetRenderDrawColor(ren, r, g, b, a);
}

} // namespace

// Shared backdrops and simple decoration.

void draw_dimmed_backdrop(SDL_Renderer* ren, std::uint8_t alpha) {
    SDL_BlendMode prev = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(ren, &prev);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    set_col(ren, 0, 0, 0, alpha);
    fill_rect(ren, 0.0f, 0.0f,
              static_cast<float>(SCR_W),
              static_cast<float>(SCR_H));
    SDL_SetRenderDrawBlendMode(ren, prev);
}

void draw_menu_backdrop(SDL_Renderer* ren) {
    // Deep blue background.
    set_col(ren, 10, 12, 24);
    SDL_RenderClear(ren);

    // Subtle starfield - just a regular grid of dim dots.
    set_col(ren, 60, 70, 100);
    for (int x = 24; x < SCR_W; x += 48) {
        for (int y = 24; y < SCR_H; y += 48) {
            // Pseudo-random "twinkle" by XORing positions.
            if (((x * 31) ^ (y * 17)) & 0x40) continue;
            fill_rect(ren,
                      static_cast<float>(x),
                      static_cast<float>(y),
                      2.0f, 2.0f);
        }
    }
}

void draw_screen_title(SDL_Renderer* ren, const std::string& title,
                       float topY) {
    set_col(ren, 90, 220, 120);
    render_text_centered(ren, title,
                         static_cast<float>(SCR_W) * 0.5f, topY, 5.0f);
}

void draw_footer(SDL_Renderer* ren, const std::string& hint) {
    set_col(ren, 140, 155, 175);
    render_text_centered(ren, hint,
                         static_cast<float>(SCR_W) * 0.5f,
                         static_cast<float>(SCR_H) - 30.0f,
                         1.5f);
}

// First-run callsign screen.

void draw_username_input(SDL_Renderer* ren,
                         const std::string& buffer,
                         bool cursorBlinkOn) {
    draw_menu_backdrop(ren);

    set_col(ren, 90, 220, 120);
    render_text_centered(ren, "SPACE INVADERS",
                         static_cast<float>(SCR_W) * 0.5f,
                         100.0f, 6.0f);

    set_col(ren, 255, 220, 120);
    render_text_centered(ren, "Pro Edition - SDL3",
                         static_cast<float>(SCR_W) * 0.5f,
                         180.0f, 2.0f);

    set_col(ren, 220, 230, 245);
    render_text_centered(ren, "Enter your callsign:",
                         static_cast<float>(SCR_W) * 0.5f,
                         270.0f, 2.0f);

    // Input box outline.
    const float boxX = static_cast<float>(SCR_W) * 0.5f - 200.0f;
    const float boxY = 310.0f;
    const float boxW = 400.0f;
    const float boxH = 50.0f;
    set_col(ren, 80, 90, 120);
    fill_rect(ren, boxX, boxY, boxW, boxH);
    set_col(ren, 130, 210, 255);
    fill_rect(ren, boxX, boxY,                 boxW, 2.0f);  // top
    fill_rect(ren, boxX, boxY + boxH - 2.0f,   boxW, 2.0f);  // bottom
    fill_rect(ren, boxX, boxY,                 2.0f, boxH);  // left
    fill_rect(ren, boxX + boxW - 2.0f, boxY,   2.0f, boxH);  // right

    // The entered text, optionally with a blinking underscore cursor.
    std::string disp = buffer;
    if (disp.empty() && cursorBlinkOn) disp = "_";
    else if (cursorBlinkOn)            disp += '_';
    set_col(ren, 220, 230, 245);
    render_text_centered(ren, disp,
                         static_cast<float>(SCR_W) * 0.5f,
                         boxY + 15.0f, 2.5f);

    draw_footer(ren, "ENTER to accept   ESC to use 'player'   max 16 chars");
}

// Main menu.

void draw_main_menu(SDL_Renderer* ren, const MenuList& menu,
                    const std::string& user, bool /*hasSave*/) {
    draw_menu_backdrop(ren);

    set_col(ren, 90, 220, 120);
    render_text_centered(ren, "SPACE INVADERS",
                         static_cast<float>(SCR_W) * 0.5f, 50.0f, 6.0f);

    set_col(ren, 255, 220, 120);
    render_text_centered(ren, "Pro Edition",
                         static_cast<float>(SCR_W) * 0.5f, 120.0f, 2.0f);

    set_col(ren, 180, 190, 210);
    std::string greet = "Pilot: " + user;
    render_text_centered(ren, greet,
                         static_cast<float>(SCR_W) * 0.5f, 165.0f, 1.5f);

    // Menu items, centered, evenly spaced.
    menu.draw(ren,
              static_cast<float>(SCR_W) * 0.5f,
              198.0f, 32.0f, 1.8f);

    draw_footer(ren,
        "UP/DOWN to move   ENTER to select   ESC to quit");
}

// Settings screen.

void draw_settings(SDL_Renderer* ren, const MenuList& menu) {
    draw_menu_backdrop(ren);
    draw_screen_title(ren, "SETTINGS");

    // Items have value-bearing labels rendered into them by the
    // caller (e.g. "Difficulty: HARD"). We just draw the list.
    menu.draw(ren,
              static_cast<float>(SCR_W) * 0.5f,
              140.0f, 36.0f, 1.8f);

    draw_footer(ren,
        "UP/DOWN to move   ENTER/LEFT/RIGHT to change   ESC to save & back");
}

// Difficulty selection.

void draw_difficulty_select(SDL_Renderer* ren, const MenuList& menu) {
    draw_menu_backdrop(ren);
    draw_screen_title(ren, "SELECT DIFFICULTY");

    menu.draw(ren,
              static_cast<float>(SCR_W) * 0.5f,
              160.0f, 44.0f, 2.0f);

    // Tip below the menu describing the highlighted difficulty.
    const int sel = menu.selected_tag();
    if (sel >= 0 && sel < N_DIFFS) {
        const auto& d = difficulty(sel);
        char buf[256];
        std::snprintf(buf, sizeof buf,
                      "Lives: %d   Move delay: %d   Player bullets: %d",
                      d.lives, d.moveDelay, d.playerBmax);
        set_col(ren, 220, 230, 245);
        render_text_centered(ren, buf,
                             static_cast<float>(SCR_W) * 0.5f,
                             420.0f, 1.5f);
        if (d.oneLife) {
            set_col(ren, 255, 100, 100);
            render_text_centered(ren,
                "!! ULTRA-NIGHTMARE: One life. No saves.",
                static_cast<float>(SCR_W) * 0.5f, 446.0f, 1.5f);
        }
    }

    draw_footer(ren, "ENTER to confirm   ESC to back");
}

// Replay filename entry.

void draw_replay_input(SDL_Renderer* ren,
                       const std::string& buffer,
                       const std::string& error,
                       bool cursorBlinkOn) {
    draw_menu_backdrop(ren);
    draw_screen_title(ren, "WATCH REPLAY");

    set_col(ren, 220, 230, 245);
    render_text_centered(ren,
        "Enter a .rpl filename from the working directory.",
        static_cast<float>(SCR_W) * 0.5f,
        145.0f, 1.7f);

    const float boxX = static_cast<float>(SCR_W) * 0.5f - 300.0f;
    const float boxY = 210.0f;
    const float boxW = 600.0f;
    const float boxH = 54.0f;
    set_col(ren, 80, 90, 120);
    fill_rect(ren, boxX, boxY, boxW, boxH);
    set_col(ren, 130, 210, 255);
    fill_rect(ren, boxX, boxY,                 boxW, 2.0f);
    fill_rect(ren, boxX, boxY + boxH - 2.0f,   boxW, 2.0f);
    fill_rect(ren, boxX, boxY,                 2.0f, boxH);
    fill_rect(ren, boxX + boxW - 2.0f, boxY,   2.0f, boxH);

    std::string disp = buffer;
    if (disp.empty() && cursorBlinkOn) disp = "_";
    else if (cursorBlinkOn)            disp += '_';
    set_col(ren, 220, 230, 245);
    render_text_centered(ren, disp,
                         static_cast<float>(SCR_W) * 0.5f,
                         boxY + 16.0f, 2.2f);

    if (!error.empty()) {
        set_col(ren, 255, 110, 110);
        render_text_centered(ren, error,
                             static_cast<float>(SCR_W) * 0.5f,
                             305.0f, 1.5f);
    } else {
        set_col(ren, 140, 155, 175);
        render_text_centered(ren,
            "Replays store inputs, not video. SDL3 will play them through the same simulation.",
            static_cast<float>(SCR_W) * 0.5f,
            305.0f, 1.2f);
    }

    draw_footer(ren, "ENTER to load   ESC to back");
}

// Leaderboard screen.

void draw_leaderboard(SDL_Renderer* ren,
                      const std::vector<Record>& lb,
                      const std::string& currentUser) {
    draw_menu_backdrop(ren);
    draw_screen_title(ren, "LEADERBOARD");

    // Header row.
    set_col(ren, 140, 155, 175);
    render_text(ren, "#",         140.0f, 130.0f, 1.5f);
    render_text(ren, "NAME",      200.0f, 130.0f, 1.5f);
    render_text(ren, "SCORE",     540.0f, 130.0f, 1.5f);
    render_text(ren, "LEVEL",     720.0f, 130.0f, 1.5f);
    render_text(ren, "DIFFICULTY", 860.0f, 130.0f, 1.5f);

    set_col(ren, 90, 220, 120);
    fill_rect(ren, 130.0f, 160.0f, 860.0f, 2.0f);

    if (lb.empty()) {
        set_col(ren, 180, 190, 210);
        render_text_centered(ren, "(no records yet - go play!)",
                             static_cast<float>(SCR_W) * 0.5f,
                             240.0f, 2.0f);
    } else {
        for (std::size_t i = 0; i < lb.size() && i < 10; ++i) {
            const float y = 175.0f + static_cast<float>(i) * 24.0f;
            const auto& r = lb[i];

            const bool isMe = (!currentUser.empty() && r.name == currentUser);
            if (isMe) {
                set_col(ren, 255, 220, 120);
            } else if (i < 3) {
                set_col(ren, 130, 210, 255);  // top 3 highlighted
            } else {
                set_col(ren, 180, 190, 210);
            }

            char num[8];
            std::snprintf(num, sizeof num, "%zu.", i + 1);
            render_text(ren, num, 140.0f, y, 1.5f);

            // Truncate long names.
            std::string nm = r.name;
            if (nm.size() > 18) nm.resize(18);
            render_text(ren, nm, 200.0f, y, 1.5f);

            char score_s[32];
            std::snprintf(score_s, sizeof score_s, "%d", r.score);
            render_text(ren, score_s, 540.0f, y, 1.5f);

            char level_s[16];
            std::snprintf(level_s, sizeof level_s, "%d", r.level);
            render_text(ren, level_s, 720.0f, y, 1.5f);

            render_text(ren, r.diff, 860.0f, y, 1.5f);
        }
    }

    draw_footer(ren, "ESC to back");
}

// Stats and achievements.

void draw_stats_achievements(SDL_Renderer* ren,
                             const std::string& user,
                             const Stats& s,
                             const std::vector<Achievement>& ach) {
    draw_menu_backdrop(ren);
    draw_screen_title(ren, "STATS & ACHIEVEMENTS");

    // Left column: lifetime stats.
    set_col(ren, 255, 220, 120);
    std::string heading = "Stats for " + user;
    render_text(ren, heading, 80.0f, 130.0f, 2.0f);

    set_col(ren, 220, 230, 245);
    const float lx = 80.0f;
    float ly = 175.0f;
    const float lstep = 22.0f;

    auto row = [&](const char* label, long val) {
        char buf[96];
        std::snprintf(buf, sizeof buf, "%-18s %ld", label, val);
        render_text(ren, buf, lx, ly, 1.5f);
        ly += lstep;
    };
    row("Games played:",  s.gamesPlayed);
    row("Total score:",   s.totalScore);
    row("Aliens killed:", s.aliensKilled);
    row("UFOs killed:",   s.ufosKilled);
    row("Bosses killed:", s.bossesKilled);
    row("Deaths:",        s.deaths);
    row("Shots fired:",   s.shotsFired);
    row("Powerups used:", s.powerupsUsed);
    row("Highest level:", s.highestLevel);
    row("Highest combo:", s.highestCombo);

    // Right column: achievements.
    set_col(ren, 255, 220, 120);
    render_text(ren, "Achievements", 600.0f, 130.0f, 2.0f);

    int unlocked = 0;
    for (const auto& a : ach) if (a.unlocked) ++unlocked;
    set_col(ren, 180, 190, 210);
    char hbuf[64];
    std::snprintf(hbuf, sizeof hbuf, "%d / %d unlocked",
                  unlocked, (int)ach.size());
    render_text(ren, hbuf, 600.0f, 158.0f, 1.5f);

    float ay = 185.0f;
    const float astep = 21.0f;
    for (const auto& a : ach) {
        if (a.unlocked) {
            set_col(ren, 130, 220, 140);
            render_text(ren, "* ", 600.0f, ay, 1.5f);
            set_col(ren, 220, 230, 245);
            render_text(ren, a.desc, 620.0f, ay, 1.5f);
        } else {
            set_col(ren, 90, 95, 105);
            render_text(ren, "- ",       600.0f, ay, 1.5f);
            render_text(ren, "(locked)", 620.0f, ay, 1.5f);
        }
        ay += astep;
        if (ay > 460.0f) break;     // run out of space
    }

    draw_footer(ren, "ESC to back");
}

// Pause overlay.

void draw_pause_overlay(SDL_Renderer* ren, const MenuList& menu,
                        const Game& g) {
    draw_dimmed_backdrop(ren, 180);

    set_col(ren, 130, 210, 255);
    render_text_centered(ren, "PAUSED",
                         static_cast<float>(SCR_W) * 0.5f,
                         110.0f, 6.0f);

    char buf[256];
    std::snprintf(buf, sizeof buf,
                  "Score: %d   Level: %d   Aliens left: %d",
                  g.player.score, g.level(), g.alien_count_alive());
    set_col(ren, 220, 230, 245);
    render_text_centered(ren, buf,
                         static_cast<float>(SCR_W) * 0.5f,
                         185.0f, 1.8f);

    menu.draw(ren,
              static_cast<float>(SCR_W) * 0.5f,
              240.0f, 38.0f, 2.0f);

    draw_footer(ren, "P to resume   UP/DOWN + ENTER on menu");
}

// Game-over overlay.

void draw_game_over(SDL_Renderer* ren, const MenuList& menu,
                    const Game& g, bool isNewBest) {
    draw_dimmed_backdrop(ren, 210);

    set_col(ren, 255, 100, 100);
    render_text_centered(ren, "GAME OVER",
                         static_cast<float>(SCR_W) * 0.5f,
                         70.0f, 7.0f);

    char buf[256];
    std::snprintf(buf, sizeof buf,
                  "Final score: %d   Level reached: %d",
                  g.player.score, g.level());
    set_col(ren, 220, 230, 245);
    render_text_centered(ren, buf,
                         static_cast<float>(SCR_W) * 0.5f,
                         170.0f, 2.0f);

    if (isNewBest) {
        set_col(ren, 255, 220, 120);
        render_text_centered(ren, "*** NEW PERSONAL BEST ***",
                             static_cast<float>(SCR_W) * 0.5f,
                             210.0f, 1.8f);
    }

    menu.draw(ren,
              static_cast<float>(SCR_W) * 0.5f,
              260.0f, 38.0f, 2.0f);

    draw_footer(ren, "UP/DOWN + ENTER   or R to restart   ESC for menu");
}

} // namespace si
