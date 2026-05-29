// main_sdl3.cpp - SDL3 windowed entry point for Space Invaders.
//
// STATE MACHINE
// =============
// The window is always in one of these screens:
//
//   USERNAME_INPUT  -> shown only on first launch (no cached name).
//                      Typing a callsign and pressing ENTER cache it to
//                      si_pro.cfg under the 'sdl3.user' key.
//
//   MAIN_MENU       -> landing screen. Lists: New Game / Resume /
//                      Difficulty / Settings / Leaderboard / Stats /
//                      AI Demo / Quit. Resume is greyed-out if no save.
//
//   DIFFICULTY_SELECT  selecting "New Game" with non-default diff goes
//                      here first.
//
//   SETTINGS        -> in-window editable Difficulty / Sound on / Music
//                      on (n/a) / Fullscreen / Language (en disabled
//                      because SDL_RenderDebugText is ASCII-only). Saves
//                      to si_pro.cfg on exit.
//
//   LEADERBOARD     -> top 10 records read from leaderboard.dat.
//
//   STATS_ACHIEVEMENTS -> lifetime stats + achievements for current user.
//
//   PLAYING         -> the actual game. Same loop as before: pre_step,
//                      step_pub, post_step, audio.observe, draw.
//                      Pressing P transitions to PAUSED.
//
//   PAUSED          -> small in-game menu: Resume / Restart / Quit-to-Menu.
//
//   GAME_OVER       -> shown when game.is_game_over(). Menu: Play Again /
//                      Main Menu / Quit. R restarts immediately.
//
//   QUIT            -> exits the loop and the program.
//
// All transitions are explicit. The main loop's job is purely
//   poll events -> dispatch to current screen -> draw current screen.

#include "core/constants.h"
#include "core/version.h"
#include "core/difficulty.h"
#include "core/rng.h"
#include "game/game.h"
#include "input/sdl3_keyboard.h"
#include "input/ai_source.h"
#include "input/input_source.h"
#include "render/sdl3_renderer.h"
#include "audio/sdl3_audio.h"
#include "ui/sdl3_menu.h"
#include "ui/sdl3_screens.h"
#include "director/director.h"
#include "config/config.h"
#include "persistence/stats.h"
#include "persistence/leaderboard.h"
#include "persistence/achievements.h"
#include "persistence/save_state.h"
#include "debug/logger.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace si {

namespace {

// ---- CLI helpers ----
bool parse_int_flag(int argc, char** argv, const char* flag, int& out) {
    for (int i = 1; i < argc - 1; ++i) {
        if (std::strcmp(argv[i], flag) == 0) {
            out = std::atoi(argv[i + 1]);
            return true;
        }
    }
    return false;
}
bool parse_str_flag(int argc, char** argv, const char* flag, std::string& out) {
    for (int i = 1; i < argc - 1; ++i) {
        if (std::strcmp(argv[i], flag) == 0) {
            out = argv[i + 1];
            return true;
        }
    }
    return false;
}
bool has_flag(int argc, char** argv, const char* flag) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], flag) == 0) return true;
    }
    return false;
}

void print_help() {
    std::printf(
        "Usage: si_pro_sdl3 [options]\n"
        "  --ai-demo        run with the AI agent instead of keyboard\n"
        "  --diff N         difficulty index 0..4 (default from config)\n"
        "  --seed N         RNG seed (default time-based)\n"
        "  --user NAME      user name for stats/leaderboard\n"
        "  --skip-menu      skip the main menu, start playing immediately\n"
        "  --fullscreen     start in fullscreen mode\n"
        "  --no-sound       do not open the audio device at all\n"
        "  --mute           start with sound muted (M toggles in-game)\n"
        "  --version        print version and exit\n"
        "  --help           this message\n"
        "\n"
        "Controls (keyboard mode in-game):\n"
        "  A / Left arrow   move left\n"
        "  D / Right arrow  move right\n"
        "  Space            shoot\n"
        "  P                pause\n"
        "  M                toggle sound mute\n"
        "  Q                quit to main menu\n"
        "  F11              toggle fullscreen\n"
        "  Esc              quit to main menu (or close window in menus)\n"
        "\n"
        "Menus: UP/DOWN to move, ENTER to select, ESC to back.\n");
}

// Random seed.
std::uint32_t time_seed() {
    return static_cast<std::uint32_t>(
        std::chrono::system_clock::now().time_since_epoch().count() & 0x7fffffff);
}

// Map diff index -> short tag for the settings menu label.
const char* diff_tag_of(int idx) {
    if (idx >= 0 && idx < N_DIFFS) return difficulty(idx).name;
    return "?";
}

// Cap username length and strip non-printable bytes (safety for the
// 8x8 ASCII debug font).
std::string sanitize_username(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c >= 0x20 && c < 0x7f && c != ' ') out.push_back(c);
        if (out.size() >= 16) break;
    }
    if (out.empty()) out = "player";
    return out;
}

// Build pause-menu items.
std::vector<MenuItem> make_pause_menu() {
    return {
        { "Resume",          0 },
        { "Restart Run",     1 },
        { "Quit to Menu",    2 },
    };
}

// Build game-over menu items.
std::vector<MenuItem> make_gameover_menu() {
    return {
        { "Play Again",      0 },
        { "Main Menu",       1 },
        { "Quit",            2 },
    };
}

// Build main menu items. 'hasSave' enables / disables "Resume".
std::vector<MenuItem> make_main_menu(bool hasSave) {
    return {
        { "New Game",                   0 },
        { "Resume Last Save",           1, hasSave },
        { "AI Demo",                    2 },
        { "Select Difficulty",          3 },
        { "Settings",                   4 },
        { "Leaderboard",                5 },
        { "Stats & Achievements",       6 },
        { "Quit",                       7 },
    };
}

// Build difficulty-select items.
std::vector<MenuItem> make_difficulty_menu() {
    std::vector<MenuItem> v;
    v.reserve(N_DIFFS + 1);
    for (int i = 0; i < N_DIFFS; ++i) {
        std::string label = std::string(difficulty(i).name);
        v.push_back({ label, i });
    }
    v.push_back({ "Back", -1 });
    return v;
}

// Draw a small "Director" status indicator in the top-right of the
// HUD. Tells the player whether the AI Director is ramping up or
// easing off, so the dynamic difficulty isn't just invisible black
// magic.
//
// Layout: a 200x20 bar at (WIN_W - 220, 4), with a colored fill
// proportional to |pressure|. Color is red when ramping up, cyan when
// easing off, green at steady. Label text above.
void draw_director_hud(SDL_Renderer* ren, const Director& dir) {
    if (!dir.enabled()) return;
    const float p = dir.pressure();    // [-1, +1]

    // Bar position (top-right of HUD).
    const float bx = static_cast<float>(SDL3Renderer::WIN_W) - 220.0f;
    const float by = 4.0f;
    const float bw = 200.0f;
    const float bh = 8.0f;

    // Background track.
    SDL_SetRenderDrawColor(ren, 60, 60, 80, 255);
    SDL_FRect bg{ bx, by + 14.0f, bw, bh };
    SDL_RenderFillRect(ren, &bg);

    // Center marker.
    SDL_SetRenderDrawColor(ren, 130, 140, 160, 255);
    SDL_FRect mid{ bx + bw * 0.5f - 1.0f, by + 14.0f, 2.0f, bh };
    SDL_RenderFillRect(ren, &mid);

    // Bar fill grows from center outward in the direction of p.
    Uint8 r = 0, g = 0, b = 0;
    if (p > 0.0f)      { r =  90; g = 220; b = 120; }   // easing - green
    else if (p < 0.0f) { r = 255; g = 110; b = 110; }   // ramping - red
    else               { r = 180; g = 190; b = 210; }   // steady - grey

    if (p != 0.0f) {
        const float half = bw * 0.5f;
        const float fillLen = half * (p < 0.0f ? -p : p);
        const float fillX   = (p > 0.0f) ? (bx + half) : (bx + half - fillLen);
        SDL_SetRenderDrawColor(ren, r, g, b, 255);
        SDL_FRect f{ fillX, by + 14.0f, fillLen, bh };
        SDL_RenderFillRect(ren, &f);
    }

    // Label "DIRECTOR: <label>" above the bar.
    SDL_SetRenderDrawColor(ren, r, g, b, 255);
    char buf[64];
    std::snprintf(buf, sizeof buf, "DIRECTOR: %s", dir.label());
    SDL_RenderDebugText(ren, bx, by, buf);
}

// Settings menu: items are rebuilt every time a value changes so the
// label reflects the new value.
std::vector<MenuItem> make_settings_menu(const Config& cfg) {
    char buf[64];
    std::vector<MenuItem> v;

    std::snprintf(buf, sizeof buf, "Difficulty: %s",
                  diff_tag_of(cfg.default_diff));
    v.push_back({ buf, 0 });

    std::snprintf(buf, sizeof buf, "Sound: %s",
                  cfg.sdl3_muted ? "OFF" : "ON");
    v.push_back({ buf, 1 });

    std::snprintf(buf, sizeof buf, "Start fullscreen: %s",
                  cfg.sdl3_fullscreen ? "ON" : "OFF");
    v.push_back({ buf, 2 });

    std::snprintf(buf, sizeof buf, "AI profile: %s",
                  cfg.ai_profile.c_str());
    v.push_back({ buf, 3 });

    std::snprintf(buf, sizeof buf, "Director AI: %s",
                  cfg.sdl3_director_enabled ? "ON" : "OFF");
    v.push_back({ buf, 4 });

    // Language: terminal-only feature; offer info-only here.
    v.push_back({ "Language: EN (Hindi terminal-only)", 5, false });

    v.push_back({ "Back", -1 });
    return v;
}

// Adjust a settings field by delta (-1 / +1) according to its tag.
void adjust_setting(Config& cfg, int tag, int delta) {
    switch (tag) {
        case 0: { // difficulty
            cfg.default_diff += delta;
            if (cfg.default_diff < 0)        cfg.default_diff = 0;
            if (cfg.default_diff >= N_DIFFS) cfg.default_diff = N_DIFFS - 1;
            break;
        }
        case 1:   cfg.sdl3_muted      = !cfg.sdl3_muted;      break;
        case 2:   cfg.sdl3_fullscreen = !cfg.sdl3_fullscreen; break;
        case 3: { // ai profile cycle
            static const char* profiles[] = { "aggressive", "balanced", "defensive" };
            int idx = 1;
            for (int i = 0; i < 3; ++i) {
                if (cfg.ai_profile == profiles[i]) { idx = i; break; }
            }
            idx = (idx + delta + 3) % 3;
            cfg.ai_profile = profiles[idx];
            break;
        }
        case 4:   cfg.sdl3_director_enabled = !cfg.sdl3_director_enabled; break;
        default: break;
    }
}

} // namespace

// =========================================================================
// Main
// =========================================================================
int main_sdl3(int argc, char** argv) {
    // ---- CLI ----
    bool aiDemoFlag  = has_flag(argc, argv, "--ai-demo");
    bool showHelp    = has_flag(argc, argv, "--help")
                    || has_flag(argc, argv, "-h");
    bool showVer     = has_flag(argc, argv, "--version");
    bool skipMenu    = has_flag(argc, argv, "--skip-menu");
    bool fullscreenF = has_flag(argc, argv, "--fullscreen");
    bool noSound     = has_flag(argc, argv, "--no-sound");
    bool startMuted  = has_flag(argc, argv, "--mute");
    int  diffIdxF    = -1;
    int  seedF       = -1;
    std::string userF;
    parse_int_flag(argc, argv, "--diff", diffIdxF);
    parse_int_flag(argc, argv, "--seed", seedF);
    parse_str_flag(argc, argv, "--user", userF);

    if (showVer) {
        std::printf("si_pro_sdl3 version %s (SDL3 build)\n", version());
        return 0;
    }
    if (showHelp) { print_help(); return 0; }

    // ---- Config ----
    Config cfg;
    load_config("si_pro.cfg", cfg);

    // Apply CLI overrides on top of config.
    if (diffIdxF >= 0)      cfg.default_diff    = std::clamp(diffIdxF, 0, N_DIFFS - 1);
    if (fullscreenF)        cfg.sdl3_fullscreen = true;
    if (startMuted)         cfg.sdl3_muted      = true;
    if (!userF.empty())     cfg.sdl3_user       = userF;

    // ---- Init SDL3 ----
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer(
            "Space Invaders - Pro Edition (SDL3)",
            SDL3Renderer::WIN_W, SDL3Renderer::WIN_H,
            SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        std::fprintf(stderr, "SDL_CreateWindowAndRenderer failed: %s\n",
                     SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderLogicalPresentation(renderer,
                                     SDL3Renderer::WIN_W,
                                     SDL3Renderer::WIN_H,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderVSync(renderer, 1);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    bool fullscreenState = cfg.sdl3_fullscreen;
    if (fullscreenState) SDL_SetWindowFullscreen(window, true);

    SDL3Renderer gfx;
    AudioSystem  audio;
    Director     director;
    if (!noSound) {
        if (!audio.init()) {
            std::fprintf(stderr,
                "warning: audio device unavailable - sound disabled (%s)\n",
                SDL_GetError());
        }
    }
    audio.set_muted(cfg.sdl3_muted);
    director.set_enabled(cfg.sdl3_director_enabled);

    // ---- Per-user persistence ----
    auto load_user_state = [&](const std::string& u, Stats& s,
                                std::vector<Achievement>& a) {
        s = stats_read(u);
        a = achievements_read(u);
        if (a.empty()) a = achievements_default();
    };
    std::string user = sanitize_username(cfg.sdl3_user);
    Stats stats;
    std::vector<Achievement> achievements;
    load_user_state(user, stats, achievements);

    // ---- App state ----
    Screen screen = Screen::MAIN_MENU;
    if (cfg.sdl3_user.empty() && userF.empty()) {
        screen = Screen::USERNAME_INPUT;
    }
    if (skipMenu) {
        screen = Screen::PLAYING;
    }
    if (aiDemoFlag) {
        screen = Screen::PLAYING;
    }

    // Username input state
    std::string typingBuf;
    Uint64 cursorBlinkStart = SDL_GetTicksNS();

    // Build initial menus.
    auto save_for_user = [&](const std::string& u) -> SaveState {
        return save_read(u);
    };
    SaveState curSave = save_for_user(user);
    MenuList mainMenu{ make_main_menu(curSave.valid) };
    MenuList settingsMenu{ make_settings_menu(cfg) };
    MenuList difficultyMenu{ make_difficulty_menu() };
    MenuList pauseMenu{ make_pause_menu() };
    MenuList gameoverMenu{ make_gameover_menu() };

    // Live game (created when entering PLAYING).
    std::unique_ptr<Game>         game;
    std::unique_ptr<IInputSource> inputP1;
    bool aiDemoActive = aiDemoFlag;

    // Track personal best so we can flag "NEW BEST" on game-over.
    int preGameBest = 0;

    auto refresh_main_menu = [&]() {
        curSave = save_for_user(user);
        mainMenu.set_items(make_main_menu(curSave.valid));
    };

    auto begin_play = [&](int diff, Mode mode, std::uint32_t seed,
                           bool fromResume, const SaveState* resumeState) {
        preGameBest = 0;
        // Personal best = current max score in leaderboard for this user.
        for (const auto& r : leaderboard_read()) {
            if (r.name == user && r.score > preGameBest) preGameBest = r.score;
        }
        if (fromResume && resumeState && resumeState->valid) {
            game = std::make_unique<Game>(*resumeState, stats, achievements);
        } else {
            game = std::make_unique<Game>(diff, mode, seed, stats, achievements);
        }
        if (mode == Mode::AI_DEMO) inputP1 = std::make_unique<AISource>();
        else                       inputP1 = std::make_unique<SDL3Keyboard>();
        gfx.on_restart(*game);
        audio.on_restart(*game);
        director.on_restart(*game);
        screen = Screen::PLAYING;
    };

    auto submit_leaderboard_if_good = [&]() {
        if (!game || game->score() <= 0) return;
        auto lb = leaderboard_read();
        Record r;
        r.name  = user;
        r.score = game->score();
        r.level = game->level();
        r.diff  = difficulty_unchecked(game->diff_idx()).name;
        lb.push_back(r);
        std::sort(lb.begin(), lb.end(),
            [](const Record& a, const Record& b){ return a.score > b.score; });
        if (lb.size() > 10) lb.resize(10);
        leaderboard_write(lb);
    };

    auto persist_all = [&]() {
        stats_write(user, stats);
        achievements_write(user, achievements);
        save_config("si_pro.cfg", cfg);
    };

    // If user requested --skip-menu / --ai-demo, build the game now.
    if (screen == Screen::PLAYING) {
        std::uint32_t seed = (seedF >= 0) ? (std::uint32_t)seedF : time_seed();
        begin_play(cfg.default_diff,
                   aiDemoFlag ? Mode::AI_DEMO : Mode::SOLO,
                   seed, false, nullptr);
    }

    // ---- Frame timing ----
    Uint64 prevNs = SDL_GetTicksNS();
    Uint64 accNs  = 0;
    constexpr Uint64 tickNs   = (Uint64)FRAME_MS * 1'000'000ull;
    constexpr Uint64 maxAccNs = 500ull * 1'000'000ull;

    bool quitRequested = false;

    // SDL3 text input lifecycle. We start it only when on USERNAME_INPUT.
    bool textInputActive = false;
    auto start_text_input = [&]() {
        if (textInputActive) return;
        SDL_StartTextInput(window);
        textInputActive = true;
    };
    auto stop_text_input = [&]() {
        if (!textInputActive) return;
        SDL_StopTextInput(window);
        textInputActive = false;
    };
    if (screen == Screen::USERNAME_INPUT) start_text_input();

    // =====================================================================
    // The main loop
    // =====================================================================
    while (!quitRequested) {
        // ---- 1. Drain events ----
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) {
                quitRequested = true;
                break;
            }
            if (ev.type == SDL_EVENT_KEY_DOWN) {
                // Global hotkeys regardless of screen
                if (ev.key.key == SDLK_F11) {
                    fullscreenState = !fullscreenState;
                    SDL_SetWindowFullscreen(window, fullscreenState);
                    continue;
                }
                if (ev.key.key == SDLK_M) {
                    audio.set_muted(!audio.muted());
                    continue;
                }

                // Per-screen dispatch
                switch (screen) {
                case Screen::USERNAME_INPUT: {
                    if (ev.key.key == SDLK_BACKSPACE) {
                        if (!typingBuf.empty()) typingBuf.pop_back();
                    } else if (ev.key.key == SDLK_RETURN
                            || ev.key.key == SDLK_KP_ENTER) {
                        user = sanitize_username(typingBuf);
                        cfg.sdl3_user = user;
                        load_user_state(user, stats, achievements);
                        refresh_main_menu();
                        stop_text_input();
                        screen = Screen::MAIN_MENU;
                        persist_all();
                    } else if (ev.key.key == SDLK_ESCAPE) {
                        user = "player";
                        cfg.sdl3_user = user;
                        load_user_state(user, stats, achievements);
                        refresh_main_menu();
                        stop_text_input();
                        screen = Screen::MAIN_MENU;
                    }
                    break;
                }
                case Screen::MAIN_MENU: {
                    auto act = mainMenu.handle_key(ev.key.key);
                    if (act == MenuList::Action::ACCEPT) {
                        switch (mainMenu.selected_tag()) {
                        case 0:   // New Game
                            begin_play(cfg.default_diff, Mode::SOLO,
                                       time_seed(), false, nullptr);
                            break;
                        case 1:   // Resume
                            if (curSave.valid) {
                                begin_play(curSave.diffIdx, Mode::SOLO,
                                           curSave.seed, true, &curSave);
                            }
                            break;
                        case 2:   // AI Demo
                            aiDemoActive = true;
                            begin_play(cfg.default_diff, Mode::AI_DEMO,
                                       time_seed(), false, nullptr);
                            break;
                        case 3:   // Difficulty
                            screen = Screen::DIFFICULTY_SELECT;
                            break;
                        case 4:   // Settings
                            settingsMenu.set_items(make_settings_menu(cfg));
                            screen = Screen::SETTINGS;
                            break;
                        case 5:   // Leaderboard
                            screen = Screen::LEADERBOARD;
                            break;
                        case 6:   // Stats
                            screen = Screen::STATS_ACHIEVEMENTS;
                            break;
                        case 7:   // Quit
                            quitRequested = true;
                            break;
                        }
                    } else if (act == MenuList::Action::CANCEL) {
                        quitRequested = true;
                    }
                    break;
                }
                case Screen::DIFFICULTY_SELECT: {
                    auto act = difficultyMenu.handle_key(ev.key.key);
                    if (act == MenuList::Action::ACCEPT) {
                        int tag = difficultyMenu.selected_tag();
                        if (tag >= 0 && tag < N_DIFFS) {
                            cfg.default_diff = tag;
                            settingsMenu.set_items(make_settings_menu(cfg));
                        }
                        screen = Screen::MAIN_MENU;
                    } else if (act == MenuList::Action::CANCEL) {
                        screen = Screen::MAIN_MENU;
                    }
                    break;
                }
                case Screen::SETTINGS: {
                    // Up/down/enter/esc come from MenuList; LEFT/RIGHT also
                    // adjust the currently-highlighted value.
                    auto act = settingsMenu.handle_key(ev.key.key);
                    if (act == MenuList::Action::ACCEPT) {
                        int tag = settingsMenu.selected_tag();
                        if (tag == -1) {
                            // "Back" item
                            audio.set_muted(cfg.sdl3_muted);
                            director.set_enabled(cfg.sdl3_director_enabled);
                            persist_all();
                            screen = Screen::MAIN_MENU;
                            break;
                        }
                        adjust_setting(cfg, tag, +1);
                        settingsMenu.set_items(make_settings_menu(cfg));
                    } else if (act == MenuList::Action::CANCEL) {
                        audio.set_muted(cfg.sdl3_muted);
                        director.set_enabled(cfg.sdl3_director_enabled);
                        persist_all();
                        screen = Screen::MAIN_MENU;
                    } else {
                        if (ev.key.key == SDLK_LEFT) {
                            adjust_setting(cfg, settingsMenu.selected_tag(), -1);
                            settingsMenu.set_items(make_settings_menu(cfg));
                        } else if (ev.key.key == SDLK_RIGHT) {
                            adjust_setting(cfg, settingsMenu.selected_tag(), +1);
                            settingsMenu.set_items(make_settings_menu(cfg));
                        }
                    }
                    break;
                }
                case Screen::LEADERBOARD: {
                    if (ev.key.key == SDLK_ESCAPE
                        || ev.key.key == SDLK_RETURN
                        || ev.key.key == SDLK_KP_ENTER) {
                        screen = Screen::MAIN_MENU;
                    }
                    break;
                }
                case Screen::STATS_ACHIEVEMENTS: {
                    if (ev.key.key == SDLK_ESCAPE
                        || ev.key.key == SDLK_RETURN
                        || ev.key.key == SDLK_KP_ENTER) {
                        screen = Screen::MAIN_MENU;
                    }
                    break;
                }
                case Screen::PLAYING: {
                    if (ev.key.key == SDLK_ESCAPE) {
                        // Treat ESC as "quit run, back to menu".
                        submit_leaderboard_if_good();
                        persist_all();
                        refresh_main_menu();
                        screen = Screen::MAIN_MENU;
                    } else if (ev.key.key == SDLK_P) {
                        // Open the pause overlay menu directly.
                        pauseMenu.set_items(make_pause_menu());
                        screen = Screen::PAUSED;
                    } else {
                        // Forward every other KEY_DOWN to the input
                        // source. SDL3Keyboard latches discrete actions
                        // (SHOOT/PAUSE/QUIT) here rather than polling
                        // SDL_GetKeyboardState(), so taps between
                        // game-ticks aren't dropped.
                        if (auto* kbd = dynamic_cast<SDL3Keyboard*>(inputP1.get())) {
                            kbd->note_key_down(ev.key.key);
                        }
                    }
                    break;
                }
                case Screen::PAUSED: {
                    auto act = pauseMenu.handle_key(ev.key.key);
                    if (act == MenuList::Action::ACCEPT) {
                        switch (pauseMenu.selected_tag()) {
                        case 0:   // Resume
                            screen = Screen::PLAYING;
                            // Reset frame timer so the accumulator doesn't
                            // catch up with a 5-second pile of ticks.
                            prevNs = SDL_GetTicksNS();
                            accNs  = 0;
                            break;
                        case 1: { // Restart Run
                            begin_play(cfg.default_diff,
                                       aiDemoActive ? Mode::AI_DEMO : Mode::SOLO,
                                       time_seed(), false, nullptr);
                            break;
                        }
                        case 2:   // Quit to Menu
                            submit_leaderboard_if_good();
                            persist_all();
                            refresh_main_menu();
                            screen = Screen::MAIN_MENU;
                            break;
                        }
                    } else if (act == MenuList::Action::CANCEL
                            || ev.key.key == SDLK_P) {
                        // ESC or P un-pauses
                        screen = Screen::PLAYING;
                        prevNs = SDL_GetTicksNS();
                        accNs  = 0;
                    }
                    break;
                }
                case Screen::GAME_OVER: {
                    auto act = gameoverMenu.handle_key(ev.key.key);
                    if (act == MenuList::Action::ACCEPT) {
                        switch (gameoverMenu.selected_tag()) {
                        case 0:   // Play Again
                            submit_leaderboard_if_good();
                            persist_all();
                            begin_play(cfg.default_diff,
                                       aiDemoActive ? Mode::AI_DEMO : Mode::SOLO,
                                       time_seed(), false, nullptr);
                            break;
                        case 1:   // Main Menu
                            submit_leaderboard_if_good();
                            persist_all();
                            refresh_main_menu();
                            screen = Screen::MAIN_MENU;
                            break;
                        case 2:   // Quit
                            submit_leaderboard_if_good();
                            persist_all();
                            quitRequested = true;
                            break;
                        }
                    } else if (ev.key.key == SDLK_R) {
                        submit_leaderboard_if_good();
                        persist_all();
                        begin_play(cfg.default_diff,
                                   aiDemoActive ? Mode::AI_DEMO : Mode::SOLO,
                                   time_seed(), false, nullptr);
                    } else if (act == MenuList::Action::CANCEL) {
                        submit_leaderboard_if_good();
                        persist_all();
                        refresh_main_menu();
                        screen = Screen::MAIN_MENU;
                    }
                    break;
                }
                default: break;
                }
            }
            if (ev.type == SDL_EVENT_TEXT_INPUT
                && screen == Screen::USERNAME_INPUT) {
                // Concatenate the typed text (up to a max length).
                for (const char* p = ev.text.text; *p && typingBuf.size() < 16; ++p) {
                    if (*p >= 0x20 && *p < 0x7f && *p != ' ') {
                        typingBuf.push_back(*p);
                    }
                }
            }
        }
        if (quitRequested) break;

        // ---- Music state follows the current screen ----
        // Music is only ON during PLAYING. Every other screen (menu,
        // pause, game-over, leaderboard, settings, etc.) gets silence
        // so the SFX (game-over jingle, menu beeps later) play clean.
        if (screen != Screen::PLAYING) {
            audio.set_music(AudioSystem::Music::NONE, 0.0f);
        }

        // ---- 2. Simulation step (only while PLAYING) ----
        Uint64 nowNs = SDL_GetTicksNS();
        Uint64 dtNs  = nowNs - prevNs;
        prevNs = nowNs;

        if (screen == Screen::PLAYING && !game->is_game_over()) {
            accNs += dtNs;
            if (accNs > maxAccNs) accNs = maxAccNs;
            while (accNs >= tickNs && !game->is_game_over()) {
                accNs -= tickNs;
                gfx.pre_step(*game);
                std::uint8_t m1 = inputP1->poll(0, *game, 0);
                std::uint8_t m2 = 0;
                game->step_pub(m1, m2);
                gfx.post_step(*game);
                audio.observe(*game);

                // Music: BOSS theme during boss waves, MARCH otherwise.
                // March intensity tracks how many aliens are left -
                // fewer aliens = closer to the bottom = higher intensity
                // and faster tempo, the classic Space Invaders trick.
                if (game->boss.active) {
                    audio.set_music(AudioSystem::Music::BOSS, 0.0f);
                } else {
                    const int total = static_cast<int>(game->aliens.size());
                    const int alive = game->alien_count_alive();
                    float intensity = 0.0f;
                    if (total > 0) {
                        intensity = 1.0f
                            - static_cast<float>(alive) / static_cast<float>(total);
                    }
                    audio.set_music(AudioSystem::Music::MARCH, intensity);
                }

                // Director: observe one tick, push fresh modifiers to
                // the Game so the NEXT tick uses them.
                const float tickSec = static_cast<float>(FRAME_MS) / 1000.0f;
                director.observe(*game, tickSec);
                const auto m = director.modifiers();
                game->set_director_modifiers(m.shootMul, m.moveMul, m.dropMul);

                game->tick_flash_decay();
                if (game->quit_flag()) {
                    submit_leaderboard_if_good();
                    persist_all();
                    refresh_main_menu();
                    audio.set_music(AudioSystem::Music::NONE, 0.0f);
                    screen = Screen::MAIN_MENU;
                    break;
                }
            }
            if (screen == Screen::PLAYING && game->is_game_over()) {
                gameoverMenu.set_items(make_gameover_menu());
                screen = Screen::GAME_OVER;
            }
        }

        // Time-based renderer state, even on menus (particles fade out
        // when you pause, etc).
        const float dtSec = static_cast<float>(dtNs) / 1.0e9f;
        gfx.tick_render(dtSec);

        // ---- 3. Draw ----
        // Render at logical 1120x512 always; logical presentation
        // letterboxes for us.
        switch (screen) {
        case Screen::USERNAME_INPUT: {
            Uint64 elapsed = SDL_GetTicksNS() - cursorBlinkStart;
            bool   blinkOn = ((elapsed / 500'000'000ull) % 2) == 0;
            draw_username_input(renderer, typingBuf, blinkOn);
            break;
        }
        case Screen::MAIN_MENU: {
            draw_main_menu(renderer, mainMenu, user, curSave.valid);
            break;
        }
        case Screen::DIFFICULTY_SELECT: {
            draw_difficulty_select(renderer, difficultyMenu);
            break;
        }
        case Screen::SETTINGS: {
            draw_settings(renderer, settingsMenu);
            break;
        }
        case Screen::LEADERBOARD: {
            draw_leaderboard(renderer, leaderboard_read(), user);
            break;
        }
        case Screen::STATS_ACHIEVEMENTS: {
            draw_stats_achievements(renderer, user, stats, achievements);
            break;
        }
        case Screen::PLAYING: {
            const float alpha = (game->is_paused() || game->is_game_over())
                              ? 1.0f
                              : static_cast<float>(accNs)
                                / static_cast<float>(tickNs);
            gfx.draw(renderer, *game, alpha);
            draw_director_hud(renderer, director);
            break;
        }
        case Screen::PAUSED: {
            // Draw the frozen game underneath, then the pause overlay.
            gfx.draw(renderer, *game, 1.0f);
            draw_director_hud(renderer, director);
            draw_pause_overlay(renderer, pauseMenu, *game);
            break;
        }
        case Screen::GAME_OVER: {
            gfx.draw(renderer, *game, 1.0f);
            draw_director_hud(renderer, director);
            const bool isNewBest = (game->score() > preGameBest);
            draw_game_over(renderer, gameoverMenu, *game, isNewBest);
            break;
        }
        default: break;
        }

        SDL_RenderPresent(renderer);
    }

    // ---- Shutdown ----
    submit_leaderboard_if_good();
    persist_all();

    audio.shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace si

int main(int argc, char** argv) {
    return si::main_sdl3(argc, argv);
}
