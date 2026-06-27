/*
    SDL3 player-facing entry point.

    Menus, replay browsing, custom levels, co-op setup, and the level editor
    live here. Actual gameplay still runs through Game::step_pub() on the same
    fixed-tick simulation used by terminal/headless tooling.
*/

#include "core/constants.h"
#include "core/version.h"
#include "core/difficulty.h"
#include "core/rng.h"
#include "game/game.h"
#include "input/sdl3_keyboard.h"
#include "input/ai_source.h"
#include "input/coop_source.h"
#include "input/input_source.h"
#include "input/replay_source.h"
#include "render/sdl3_renderer.h"
#include "audio/sdl3_audio.h"
#include "ui/sdl3_file_browser.h"
#include "ui/sdl3_menu.h"
#include "ui/sdl3_screens.h"
#include "director/director.h"
#include "config/config.h"
#include "persistence/stats.h"
#include "persistence/leaderboard.h"
#include "persistence/achievements.h"
#include "persistence/replay_file.h"
#include "persistence/level_file.h"
#include "persistence/save_state.h"
#include "net/tcp_socket.h"
#include "platform/platform.h"
#include "debug/logger.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <future>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace si {

namespace {

constexpr int REPLAY_BROWSER_MANUAL = -1000;

struct CoopConnectResult {
    net::TCPSocket socket;
    std::uint32_t seed = 0;
    int diffIdx = 0;
    int selfPlayer = 0;
    bool ok = false;
    std::string error;
};

enum class LevelEditorTextField {
    NONE,
    NAME,
    AUTHOR,
    SEED,
    MOVE_DELAY,
    SHOOT_BASE,
    SAVE_PATH
};

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

std::uint32_t time_seed() {
    return static_cast<std::uint32_t>(
        std::chrono::system_clock::now().time_since_epoch().count() & 0x7fffffff);
}

const char* diff_tag_of(int idx) {
    if (idx >= 0 && idx < N_DIFFS) return difficulty(idx).name;
    return "?";
}

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

std::string sanitize_replay_path(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c >= 0x20 && c < 0x7f) out.push_back(c);
        if (out.size() >= 96) break;
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    while (!out.empty() && out.front() == ' ') out.erase(out.begin());
    const std::size_t sep = out.find_last_of("/\\");
    const std::size_t dot = out.find_last_of('.');
    const bool hasExt = dot != std::string::npos
        && (sep == std::string::npos || dot > sep);
    if (!out.empty() && !hasExt) out += ".rpl";
    return out;
}

bool replay_path_is_absolute(const std::string& path) {
    if (path.empty()) return false;
    if (path[0] == '/' || path[0] == '\\') return true;
    return path.size() >= 3
        && ((path[0] >= 'A' && path[0] <= 'Z')
            || (path[0] >= 'a' && path[0] <= 'z'))
        && path[1] == ':'
        && (path[2] == '/' || path[2] == '\\');
}

void add_replay_candidate(std::vector<std::string>& out,
                          const std::string& path) {
    if (path.empty()) return;
    if (std::find(out.begin(), out.end(), path) == out.end()) {
        out.push_back(path);
    }
}

std::vector<std::string> replay_load_candidates(const std::string& path) {
    std::vector<std::string> out;
    add_replay_candidate(out, path);
    if (replay_path_is_absolute(path)) return out;

    add_replay_candidate(out, "build/" + path);
    add_replay_candidate(out, "../" + path);

    const char* base = SDL_GetBasePath();
    if (base && *base) {
        std::string baseDir = base;
        const char last = baseDir.empty() ? '\0' : baseDir.back();
        if (last != '/' && last != '\\') baseDir += '/';
        add_replay_candidate(out, baseDir + path);
        add_replay_candidate(out, baseDir + "../" + path);
    }
    return out;
}

FileBrowserOptions replay_browser_options() {
    FileBrowserOptions options;
    options.extension = ".rpl";
    options.emptyLabel = "(no replay files found)";
    options.relativeDirs = { "build" };
    options.includeCwdParent = true;
    options.includeExeParent = true;
    return options;
}

FileBrowserOptions level_browser_options() {
    FileBrowserOptions options;
    options.extension = ".lvl";
    options.emptyLabel = "(no level files found)";
    options.relativeDirs = { "levels", "build" };
    options.includeExeParent = true;
    options.includeBuildPrefixedDirs = true;
    return options;
}

std::vector<MenuItem> make_pause_menu() {
    return {
        { "Resume",          0 },
        { "Restart Run",     1 },
        { "Quit to Menu",    2 },
    };
}

std::vector<MenuItem> make_gameover_menu() {
    return {
        { "Play Again",      0 },
        { "Main Menu",       1 },
        { "Quit",            2 },
    };
}

std::vector<MenuItem> make_replay_over_menu() {
    return {
        { "Watch Again",     0 },
        { "Main Menu",       1 },
        { "Quit",            2 },
    };
}

std::vector<MenuItem> make_coop_menu() {
    return {
        { "Host Game", 0 },
        { "Join Game", 1 },
        { "Back",      2 },
    };
}

std::vector<MenuItem> make_level_preview_menu() {
    return {
        { "Play", 0 },
        { "Edit", 1 },
        { "Back", 2 },
    };
}

std::vector<MenuItem> make_main_menu(bool hasSave) {
    return {
        { "New Game",                   0 },
        { "Resume Last Save",           1, hasSave },
        { "AI Demo",                    2 },
        { "Co-op",                      3 },
        { "Custom Levels",              4 },
        { "Level Editor",               5 },
        { "Watch Replay",               6 },
        { "Select Difficulty",          7 },
        { "Settings",                   8 },
        { "Leaderboard",                9 },
        { "Stats & Achievements",      10 },
        { "Quit",                      11 },
    };
}

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

void draw_director_hud(SDL_Renderer* ren, const Director& dir) {
    if (!dir.enabled()) return;
    const float p = dir.pressure();

    const float bx = static_cast<float>(SDL3Renderer::WIN_W) - 220.0f;
    const float by = 4.0f;
    const float bw = 200.0f;
    const float bh = 8.0f;

    SDL_SetRenderDrawColor(ren, 60, 60, 80, 255);
    SDL_FRect bg{ bx, by + 14.0f, bw, bh };
    SDL_RenderFillRect(ren, &bg);

    SDL_SetRenderDrawColor(ren, 130, 140, 160, 255);
    SDL_FRect mid{ bx + bw * 0.5f - 1.0f, by + 14.0f, 2.0f, bh };
    SDL_RenderFillRect(ren, &mid);

    Uint8 r = 0, g = 0, b = 0;
    if (p > 0.0f)      { r =  90; g = 220; b = 120; }
    else if (p < 0.0f) { r = 255; g = 110; b = 110; }
    else               { r = 180; g = 190; b = 210; }

    if (p != 0.0f) {
        const float half = bw * 0.5f;
        const float fillLen = half * (p < 0.0f ? -p : p);
        const float fillX   = (p > 0.0f) ? (bx + half) : (bx + half - fillLen);
        SDL_SetRenderDrawColor(ren, r, g, b, 255);
        SDL_FRect f{ fillX, by + 14.0f, fillLen, bh };
        SDL_RenderFillRect(ren, &f);
    }

    SDL_SetRenderDrawColor(ren, r, g, b, 255);
    char buf[64];
    std::snprintf(buf, sizeof buf, "DIRECTOR: %s", dir.label());
    SDL_RenderDebugText(ren, bx, by, buf);

    if (dir.beat_active()) {
        const float panelY = by + 30.0f;
        const float panelH = 28.0f;
        SDL_SetRenderDrawColor(ren, 10, 12, 24, 220);
        SDL_FRect panel{ bx - 20.0f, panelY, bw + 20.0f, panelH };
        SDL_RenderFillRect(ren, &panel);

        SDL_SetRenderDrawColor(ren, r, g, b, 255);
        std::snprintf(buf, sizeof buf, "%s  %02ds",
                      dir.beat_label(),
                      static_cast<int>(dir.beat_seconds_left() + 0.99f));
        SDL_RenderDebugText(ren, bx - 12.0f, panelY + 4.0f, buf);

        SDL_SetRenderDrawColor(ren, 50, 55, 72, 255);
        SDL_FRect track{ bx - 12.0f, panelY + 20.0f, bw, 4.0f };
        SDL_RenderFillRect(ren, &track);

        SDL_SetRenderDrawColor(ren, r, g, b, 255);
        SDL_FRect fill{ bx - 12.0f, panelY + 20.0f,
                        bw * dir.beat_progress(), 4.0f };
        SDL_RenderFillRect(ren, &fill);
    }
}

void draw_replay_hud(SDL_Renderer* ren, const Replay& rp,
                     const std::string& path) {
    SDL_SetRenderDrawColor(ren, 255, 220, 120, 255);
    SDL_RenderDebugText(ren, 14.0f, 82.0f, "REPLAY MODE");

    char buf[192];
    std::snprintf(buf, sizeof buf, "file=%s  player=%s  seed=%u",
                  path.c_str(),
                  rp.player.empty() ? "unknown" : rp.player.c_str(),
                  rp.seed);
    SDL_SetRenderDrawColor(ren, 180, 190, 210, 255);
    SDL_RenderDebugText(ren, 120.0f, 82.0f, buf);
}

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

    std::snprintf(buf, sizeof buf, "Reduced motion: %s",
                  cfg.sdl3_reduced_motion ? "ON" : "OFF");
    v.push_back({ buf, 5 });

    v.push_back({ "Language: EN (Hindi terminal-only)", 6, false });

    v.push_back({ "Back", -1 });
    return v;
}

void adjust_setting(Config& cfg, int tag, int delta) {
    switch (tag) {
        case 0: {
            cfg.default_diff += delta;
            if (cfg.default_diff < 0)        cfg.default_diff = 0;
            if (cfg.default_diff >= N_DIFFS) cfg.default_diff = N_DIFFS - 1;
            break;
        }
        case 1:   cfg.sdl3_muted      = !cfg.sdl3_muted;      break;
        case 2:   cfg.sdl3_fullscreen = !cfg.sdl3_fullscreen; break;
        case 3: {
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
        case 5:   cfg.sdl3_reduced_motion   = !cfg.sdl3_reduced_motion;   break;
        default: break;
    }
}

}

int main_sdl3(int argc, char** argv) {

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

    Config cfg;
    load_config("si_pro.cfg", cfg);

    if (diffIdxF >= 0)      cfg.default_diff    = std::clamp(diffIdxF, 0, N_DIFFS - 1);
    if (fullscreenF)        cfg.sdl3_fullscreen = true;
    if (startMuted)         cfg.sdl3_muted      = true;
    if (!userF.empty())     cfg.sdl3_user       = userF;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer(
            "Space Invaders - Pro Edition",
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
    platform::net_init();

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
    gfx.set_reduced_motion(cfg.sdl3_reduced_motion);

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

    std::string typingBuf;
    std::string replayPathBuf;
    std::string replayError;
    std::string coopJoinBuf = "127.0.0.1";
    std::string coopError;
    std::string coopStatus;
    std::string editorTextBuf;
    std::string editorTextError;
    std::string editorTextLabel;
    Uint64 cursorBlinkStart = SDL_GetTicksNS();

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

    auto save_for_user = [&](const std::string& u) -> SaveState {
        return save_read(u);
    };
    SaveState curSave = save_for_user(user);
    MenuList mainMenu{ make_main_menu(curSave.valid) };
    MenuList settingsMenu{ make_settings_menu(cfg) };
    MenuList difficultyMenu{ make_difficulty_menu() };
    MenuList coopMenu{ make_coop_menu() };
    MenuList levelBrowserMenu;
    MenuList levelPreviewMenu{ make_level_preview_menu() };
    MenuList replayBrowserMenu;
    MenuList pauseMenu{ make_pause_menu() };
    MenuList gameoverMenu{ make_gameover_menu() };

    std::unique_ptr<Game>         game;
    std::unique_ptr<IInputSource> inputP1;
    std::unique_ptr<IInputSource> inputP2;
    bool aiDemoActive = aiDemoFlag;
    bool replayActive = false;
    bool customLevelActive = false;
    bool coopActive = false;
    bool replaySavedThisRun = false;
    Replay replayData;
    std::string replayPath;
    std::vector<FileBrowserItem> replayChoices;
    ReplaySummaryView replaySummary;
    std::vector<FileBrowserItem> levelChoices;
    LevelFile levelPreview;
    std::string levelPreviewPath;
    LevelFile editorLevel;
    std::string editorPath = "custom_level.lvl";
    std::string editorMessage;
    LevelEditorTextField editorTextField = LevelEditorTextField::NONE;
    int editorGrid = 0;
    int editorAlienRow = 0;
    int editorAlienCol = 0;
    int editorShieldRow = 0;
    int editorShieldCol = 0;
    std::string levelError;
    std::string levelErrorPath;
    std::string customLevelPath;
    Stats replayStats;
    std::vector<Achievement> replayAchievements;
    std::unique_ptr<net::TCPSocket> coopSocket;
    std::unique_ptr<SDL3Keyboard> coopKeyboard;
    std::atomic<bool> coopDead{ false };
    std::future<CoopConnectResult> coopFuture;

    int preGameBest = 0;

    auto refresh_main_menu = [&]() {
        curSave = save_for_user(user);
        mainMenu.set_items(make_main_menu(curSave.valid));
    };

    auto refresh_replay_browser = [&]() {
        const auto options = replay_browser_options();
        replayChoices = find_browser_files(options);
        replayBrowserMenu.set_items(make_file_browser_menu(
            replayChoices, options, { { "Type filename...", REPLAY_BROWSER_MANUAL } }));
    };

    auto refresh_level_browser = [&]() {
        const auto options = level_browser_options();
        levelChoices = find_browser_files(options);
        levelBrowserMenu.set_items(make_file_browser_menu(levelChoices, options));
    };

    auto open_level_preview = [&](const std::string& path) -> bool {
        LevelFile level;
        if (!level_load(path, level)) {
            levelErrorPath = path;
            levelError = "Could not load level file.";
            return false;
        }
        levelPreview = level;
        levelPreviewPath = path;
        levelPreviewMenu.set_items(make_level_preview_menu());
        levelError.clear();
        screen = Screen::LEVEL_PREVIEW;
        return true;
    };

    auto sanitize_level_path = [](std::string path) {
        while (!path.empty() && path.back() == ' ') path.pop_back();
        while (!path.empty() && path.front() == ' ') path.erase(path.begin());
        if (path.empty()) path = "custom_level.lvl";
        const std::size_t sep = path.find_last_of("/\\");
        const std::size_t dot = path.find_last_of('.');
        const bool hasExt = dot != std::string::npos
            && (sep == std::string::npos || dot > sep);
        if (!hasExt) path += ".lvl";
        return path;
    };

    auto start_editor = [&](const LevelFile& level, const std::string& path) {
        editorLevel = level;
        editorPath = path.empty() ? "custom_level.lvl" : path;
        editorMessage.clear();
        editorTextError.clear();
        editorTextField = LevelEditorTextField::NONE;
        editorGrid = 0;
        editorAlienRow = 0;
        editorAlienCol = 0;
        editorShieldRow = 0;
        editorShieldCol = 0;
        screen = Screen::LEVEL_EDITOR;
    };

    auto start_editor_text = [&](LevelEditorTextField field,
                                 const std::string& label,
                                 const std::string& value) {
        editorTextField = field;
        editorTextLabel = label;
        editorTextBuf = value;
        editorTextError.clear();
        start_text_input();
        screen = Screen::LEVEL_EDITOR_TEXT_INPUT;
    };

    auto save_editor_level = [&]() {
        editorPath = sanitize_level_path(editorPath);
        if (level_save(editorPath, editorLevel)) {
            editorMessage = "Saved " + editorPath;
        } else {
            editorMessage = "Save failed: " + editorPath;
        }
    };

    auto apply_editor_text = [&]() -> bool {
        try {
            switch (editorTextField) {
            case LevelEditorTextField::NAME:
                editorLevel.name = editorTextBuf.empty() ? "Untitled" : editorTextBuf;
                break;
            case LevelEditorTextField::AUTHOR:
                editorLevel.author = editorTextBuf.empty() ? user : editorTextBuf;
                break;
            case LevelEditorTextField::SEED:
                editorLevel.seed = static_cast<std::uint32_t>(std::stoul(editorTextBuf));
                break;
            case LevelEditorTextField::MOVE_DELAY:
                editorLevel.moveDelay = std::max(1, std::stoi(editorTextBuf));
                break;
            case LevelEditorTextField::SHOOT_BASE:
                editorLevel.shootBase = std::max(1, std::stoi(editorTextBuf));
                break;
            case LevelEditorTextField::SAVE_PATH:
                editorPath = sanitize_level_path(editorTextBuf);
                save_editor_level();
                break;
            case LevelEditorTextField::NONE:
                break;
            }
        } catch (...) {
            editorTextError = "Invalid value.";
            return false;
        }
        editorTextField = LevelEditorTextField::NONE;
        stop_text_input();
        screen = Screen::LEVEL_EDITOR;
        return true;
    };

    auto clear_coop_connection = [&]() {
        inputP1.reset();
        inputP2.reset();
        coopKeyboard.reset();
        coopSocket.reset();
        coopActive = false;
        coopDead.store(false);
    };

    auto start_coop_host = [&]() {
        coopError.clear();
        coopStatus = "Waiting for a client on port "
                   + std::to_string(cfg.net_port) + "...";
        const int diff = cfg.default_diff;
        const int port = cfg.net_port;
        const std::uint32_t seed = time_seed();
        coopFuture = std::async(std::launch::async, [diff, port, seed]() mutable {
            CoopConnectResult result;
            result.seed = seed;
            result.diffIdx = diff;
            result.selfPlayer = 0;

            net::TCPSocket sock = net::net_host(port, 30);
            if (!sock.valid()) {
                result.error = "Could not host. Port may be busy or timed out.";
                return result;
            }

            std::stringstream hello;
            hello << "HELLO " << seed << ' ' << diff;
            if (!sock.sendLine(hello.str())) {
                result.error = "Handshake send failed.";
                return result;
            }

            std::string reply;
            if (!sock.recvLine(reply) || reply != "OK") {
                result.error = "Handshake failed.";
                return result;
            }

            result.socket = std::move(sock);
            result.ok = true;
            return result;
        });
        screen = Screen::COOP_CONNECTING;
    };

    auto start_coop_join = [&](const std::string& ip) {
        coopError.clear();
        coopStatus = "Connecting to " + ip + ":" + std::to_string(cfg.net_port) + "...";
        const int port = cfg.net_port;
        coopFuture = std::async(std::launch::async, [ip, port]() mutable {
            CoopConnectResult result;
            result.selfPlayer = 1;

            net::TCPSocket sock = net::net_join(ip, port);
            if (!sock.valid()) {
                result.error = "Could not connect to host.";
                return result;
            }

            std::string hello;
            if (!sock.recvLine(hello) || hello.rfind("HELLO ", 0) != 0) {
                result.error = "Bad host handshake.";
                return result;
            }

            std::stringstream in(hello);
            std::string tag;
            in >> tag >> result.seed >> result.diffIdx;
            if (tag != "HELLO"
                || result.diffIdx < 0
                || result.diffIdx >= N_DIFFS) {
                result.error = "Host sent invalid game settings.";
                return result;
            }

            if (!sock.sendLine("OK")) {
                result.error = "Handshake reply failed.";
                return result;
            }

            result.socket = std::move(sock);
            result.ok = true;
            return result;
        });
        screen = Screen::COOP_CONNECTING;
    };

    auto begin_coop_game = [&](CoopConnectResult result) {
        clear_coop_connection();
        replayActive = false;
        customLevelActive = false;
        replaySavedThisRun = false;
        replayPath.clear();
        customLevelPath.clear();
        aiDemoActive = false;
        preGameBest = 0;

        coopSocket = std::make_unique<net::TCPSocket>(std::move(result.socket));
        coopKeyboard = std::make_unique<SDL3Keyboard>();
        coopDead.store(false);
        coopActive = true;

        const Mode mode = result.selfPlayer == 0
            ? Mode::COOP_HOST
            : Mode::COOP_CLIENT;
        game = std::make_unique<Game>(result.diffIdx, mode,
                                      result.seed, stats, achievements);
        inputP1 = std::make_unique<CoopSource>(
            *coopSocket, *coopKeyboard, coopDead, result.selfPlayer);
        inputP2 = std::make_unique<CoopSource>(
            *coopSocket, *coopKeyboard, coopDead, result.selfPlayer);

        director.set_enabled(cfg.sdl3_director_enabled);
        gfx.on_restart(*game);
        audio.on_restart(*game);
        director.on_restart(*game);
        screen = Screen::PLAYING;
    };

    auto begin_play = [&](int diff, Mode mode, std::uint32_t seed,
                           bool fromResume, const SaveState* resumeState) {
        clear_coop_connection();
        replayActive = false;
        customLevelActive = false;
        replaySavedThisRun = false;
        replayPath.clear();
        customLevelPath.clear();
        inputP2.reset();
        director.set_enabled(cfg.sdl3_director_enabled);
        preGameBest = 0;

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

    auto begin_custom_level = [&](const std::string& path,
                                  std::string& err) -> bool {
        clear_coop_connection();
        LevelFile level;
        if (!level_load(path, level)) {
            levelErrorPath = path;
            err = "Could not load level file.";
            return false;
        }

        replayActive = false;
        customLevelActive = true;
        replaySavedThisRun = true;
        replayPath.clear();
        customLevelPath = path;
        inputP2.reset();
        aiDemoActive = false;
        preGameBest = 0;

        game = std::make_unique<Game>(cfg.default_diff, Mode::SOLO,
                                      level, stats, achievements);
        inputP1 = std::make_unique<SDL3Keyboard>();

        director.set_enabled(cfg.sdl3_director_enabled);
        gfx.on_restart(*game);
        audio.on_restart(*game);
        director.on_restart(*game);
        screen = Screen::PLAYING;
        err.clear();
        return true;
    };

    auto begin_replay = [&](const std::string& requestedPath,
                            std::string& err) -> bool {
        clear_coop_connection();
        const std::string path = sanitize_replay_path(requestedPath);
        if (path.empty()) {
            err = "Enter a replay filename.";
            return false;
        }

        Replay loaded;
        std::string resolvedPath;
        for (const auto& candidate : replay_load_candidates(path)) {
            Replay attempt;
            if (replay_load(candidate, attempt)) {
                loaded = attempt;
                resolvedPath = candidate;
                break;
            }
        }
        if (resolvedPath.empty()) {
            err = "Could not load replay: " + path;
            return false;
        }
        if (loaded.diffIdx < 0 || loaded.diffIdx >= N_DIFFS) {
            err = "Replay has an invalid difficulty index.";
            return false;
        }
        if (loaded.frames.empty()) {
            err = "Replay has no input frames.";
            return false;
        }

        replayData = loaded;
        replayPath = resolvedPath;
        customLevelPath.clear();
        replayStats = Stats{};
        replayAchievements = achievements_default();
        replayActive = true;
        customLevelActive = false;
        replaySavedThisRun = true;
        aiDemoActive = false;
        preGameBest = 0;

        game = std::make_unique<Game>(replayData.diffIdx, Mode::REPLAY,
                                      replayData.seed,
                                      replayStats, replayAchievements);
        inputP1 = std::make_unique<ReplaySource>(replayData.frames, 1);
        inputP2 = std::make_unique<ReplaySource>(replayData.frames, 2);

        director.set_enabled(false);
        game->set_director_modifiers(1.0f, 1.0f, 1.0f);
        gfx.on_restart(*game);
        audio.on_restart(*game);
        director.on_restart(*game);
        screen = Screen::PLAYING;
        err.clear();
        return true;
    };

    auto build_replay_summary = [&]() {
        ReplaySummaryView summary;
        summary.file = replayPath;
        summary.seed = replayData.seed;
        summary.difficulty = difficulty(replayData.diffIdx).name;
        summary.mode = replayData.modeStr;
        summary.player = replayData.player;
        summary.expectedScore = replayData.expectedScore;
        summary.expectedLevel = replayData.expectedLevel;
        summary.actualScore = game ? game->score() : 0;
        summary.actualLevel = game ? game->level() : 0;

        const bool hasExpected = summary.expectedScore >= 0
                              || summary.expectedLevel >= 0;
        const bool scoreOk = summary.expectedScore < 0
                          || summary.expectedScore == summary.actualScore;
        const bool levelOk = summary.expectedLevel < 0
                          || summary.expectedLevel == summary.actualLevel;
        summary.status = hasExpected
            ? ((scoreOk && levelOk) ? "PASS" : "FAIL")
            : "NO EXPECTED RESULT";
        return summary;
    };

    auto clear_finished_replay = [&]() {
        game.reset();
        inputP1.reset();
        inputP2.reset();
        replayActive = false;
        replaySavedThisRun = true;
    };

    auto submit_leaderboard_if_good = [&]() {
        if (replayActive || customLevelActive || !game || game->score() <= 0) return;
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

    auto save_last_replay_if_needed = [&]() {
        if (replaySavedThisRun || replayActive || customLevelActive || !game) return;
        if (game->replay().frames.empty()) return;

        Replay rp = game->replay();
        rp.player = aiDemoActive ? user + "_AI" : user;
        rp.expectedScore = game->score();
        rp.expectedLevel = game->level();

        const std::string path = aiDemoActive
            ? user + "_ai_last.rpl"
            : user + "_last.rpl";
        replay_save(path, rp);
        replaySavedThisRun = true;
    };

    auto persist_all = [&]() {
        stats_write(user, stats);
        achievements_write(user, achievements);
        save_config("si_pro.cfg", cfg);
    };

    if (screen == Screen::PLAYING) {
        std::uint32_t seed = (seedF >= 0) ? (std::uint32_t)seedF : time_seed();
        begin_play(cfg.default_diff,
                   aiDemoFlag ? Mode::AI_DEMO : Mode::SOLO,
                   seed, false, nullptr);
    }

    Uint64 prevNs = SDL_GetTicksNS();
    Uint64 accNs  = 0;
    constexpr Uint64 tickNs   = (Uint64)FRAME_MS * 1'000'000ull;
    constexpr Uint64 maxAccNs = 500ull * 1'000'000ull;

    bool quitRequested = false;

    if (screen == Screen::USERNAME_INPUT) start_text_input();

    while (!quitRequested) {

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) {
                quitRequested = true;
                break;
            }
            if (ev.type == SDL_EVENT_KEY_DOWN) {

                if (ev.key.key == SDLK_F11) {
                    fullscreenState = !fullscreenState;
                    SDL_SetWindowFullscreen(window, fullscreenState);
                    continue;
                }
                if (ev.key.key == SDLK_M) {
                    audio.set_muted(!audio.muted());
                    continue;
                }

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
                        case 0:
                            begin_play(cfg.default_diff, Mode::SOLO,
                                       time_seed(), false, nullptr);
                            break;
                        case 1:
                            if (curSave.valid) {
                                begin_play(curSave.diffIdx, Mode::SOLO,
                                           curSave.seed, true, &curSave);
                            }
                            break;
                        case 2:
                            aiDemoActive = true;
                            begin_play(cfg.default_diff, Mode::AI_DEMO,
                                       time_seed(), false, nullptr);
                            break;
                        case 3:
                            coopError.clear();
                            coopMenu.set_items(make_coop_menu());
                            screen = Screen::COOP_MENU;
                            break;
                        case 4:
                            levelError.clear();
                            levelErrorPath.clear();
                            refresh_level_browser();
                            screen = Screen::CUSTOM_LEVELS;
                            break;
                        case 5:
                            start_editor(LevelFile{}, "custom_level.lvl");
                            break;
                        case 6:
                            replayPathBuf.clear();
                            replayError.clear();
                            refresh_replay_browser();
                            screen = Screen::REPLAY_BROWSER;
                            break;
                        case 7:
                            screen = Screen::DIFFICULTY_SELECT;
                            break;
                        case 8:
                            settingsMenu.set_items(make_settings_menu(cfg));
                            screen = Screen::SETTINGS;
                            break;
                        case 9:
                            screen = Screen::LEADERBOARD;
                            break;
                        case 10:
                            screen = Screen::STATS_ACHIEVEMENTS;
                            break;
                        case 11:
                            quitRequested = true;
                            break;
                        }
                    } else if (act == MenuList::Action::CANCEL) {
                        quitRequested = true;
                    }
                    break;
                }
                case Screen::COOP_MENU: {
                    auto act = coopMenu.handle_key(ev.key.key);
                    if (act == MenuList::Action::ACCEPT) {
                        switch (coopMenu.selected_tag()) {
                        case 0:
                            start_coop_host();
                            break;
                        case 1:
                            coopJoinBuf = "127.0.0.1";
                            coopError.clear();
                            start_text_input();
                            screen = Screen::COOP_JOIN_INPUT;
                            break;
                        case 2:
                            screen = Screen::MAIN_MENU;
                            break;
                        }
                    } else if (act == MenuList::Action::CANCEL) {
                        screen = Screen::MAIN_MENU;
                    }
                    break;
                }
                case Screen::COOP_JOIN_INPUT: {
                    if (ev.key.key == SDLK_BACKSPACE) {
                        if (!coopJoinBuf.empty()) coopJoinBuf.pop_back();
                    } else if (ev.key.key == SDLK_RETURN
                            || ev.key.key == SDLK_KP_ENTER) {
                        if (coopJoinBuf.empty()) {
                            coopError = "Enter an IP address.";
                        } else {
                            stop_text_input();
                            start_coop_join(coopJoinBuf);
                        }
                    } else if (ev.key.key == SDLK_ESCAPE) {
                        stop_text_input();
                        coopError.clear();
                        screen = Screen::COOP_MENU;
                    }
                    break;
                }
                case Screen::COOP_CONNECTING:
                    break;
                case Screen::COOP_ERROR: {
                    if (ev.key.key == SDLK_ESCAPE
                        || ev.key.key == SDLK_RETURN
                        || ev.key.key == SDLK_KP_ENTER) {
                        coopMenu.set_items(make_coop_menu());
                        screen = Screen::COOP_MENU;
                    }
                    break;
                }
                case Screen::CUSTOM_LEVELS: {
                    auto act = levelBrowserMenu.handle_key(ev.key.key);
                    if (act == MenuList::Action::ACCEPT) {
                        const int tag = levelBrowserMenu.selected_tag();
                        if (tag >= 0
                            && tag < static_cast<int>(levelChoices.size())) {
                            if (!open_level_preview(levelChoices[tag].path)) {
                                screen = Screen::LEVEL_LOAD_ERROR;
                            }
                        } else if (tag == FILE_BROWSER_BACK) {
                            levelError.clear();
                            screen = Screen::MAIN_MENU;
                        }
                    } else if (act == MenuList::Action::CANCEL) {
                        levelError.clear();
                        screen = Screen::MAIN_MENU;
                    }
                    break;
                }
                case Screen::LEVEL_PREVIEW: {
                    auto act = levelPreviewMenu.handle_key(ev.key.key);
                    if (act == MenuList::Action::ACCEPT) {
                        switch (levelPreviewMenu.selected_tag()) {
                        case 0:
                            if (begin_custom_level(levelPreviewPath, levelError)) {
                                prevNs = SDL_GetTicksNS();
                                accNs = 0;
                            } else {
                                screen = Screen::LEVEL_LOAD_ERROR;
                            }
                            break;
                        case 1:
                            start_editor(levelPreview, levelPreviewPath);
                            break;
                        case 2:
                            refresh_level_browser();
                            screen = Screen::CUSTOM_LEVELS;
                            break;
                        }
                    } else if (act == MenuList::Action::CANCEL) {
                        refresh_level_browser();
                        screen = Screen::CUSTOM_LEVELS;
                    }
                    break;
                }
                case Screen::LEVEL_EDITOR: {
                    const auto key = ev.key.key;
                    if (key == SDLK_ESCAPE) {
                        refresh_level_browser();
                        screen = Screen::CUSTOM_LEVELS;
                    } else if (key == SDLK_TAB) {
                        editorGrid = 1 - editorGrid;
                    } else if (key == SDLK_LEFT) {
                        if (editorGrid == 0) editorAlienCol = std::max(0, editorAlienCol - 1);
                        else                 editorShieldCol = std::max(0, editorShieldCol - 1);
                    } else if (key == SDLK_RIGHT) {
                        if (editorGrid == 0) editorAlienCol = std::min(ACOLS - 1, editorAlienCol + 1);
                        else                 editorShieldCol = std::min(3, editorShieldCol + 1);
                    } else if (key == SDLK_UP) {
                        if (editorGrid == 0) editorAlienRow = std::max(0, editorAlienRow - 1);
                        else                 editorShieldRow = std::max(0, editorShieldRow - 1);
                    } else if (key == SDLK_DOWN) {
                        if (editorGrid == 0) editorAlienRow = std::min(AROWS - 1, editorAlienRow + 1);
                        else                 editorShieldRow = std::min(1, editorShieldRow + 1);
                    } else if (key == SDLK_SPACE) {
                        if (editorGrid == 0) {
                            bool& cell = editorLevel.aliens[editorAlienRow][editorAlienCol];
                            cell = !cell;
                        } else {
                            bool& cell = editorLevel.shield[editorShieldRow][editorShieldCol];
                            cell = !cell;
                        }
                    } else if (key == SDLK_B) {
                        editorLevel.boss = !editorLevel.boss;
                    } else if (key == SDLK_N) {
                        start_editor_text(LevelEditorTextField::NAME,
                                          "Level name", editorLevel.name);
                    } else if (key == SDLK_A) {
                        start_editor_text(LevelEditorTextField::AUTHOR,
                                          "Author", editorLevel.author);
                    } else if (key == SDLK_E) {
                        start_editor_text(LevelEditorTextField::SEED,
                                          "Seed", std::to_string(editorLevel.seed));
                    } else if (key == SDLK_M) {
                        start_editor_text(LevelEditorTextField::MOVE_DELAY,
                                          "Alien move delay", std::to_string(editorLevel.moveDelay));
                    } else if (key == SDLK_H) {
                        start_editor_text(LevelEditorTextField::SHOOT_BASE,
                                          "Alien shoot delay", std::to_string(editorLevel.shootBase));
                    } else if (key == SDLK_F) {
                        start_editor_text(LevelEditorTextField::SAVE_PATH,
                                          "Save as .lvl", editorPath);
                    } else if (key == SDLK_S) {
                        save_editor_level();
                    } else if (key == SDLK_P) {
                        save_editor_level();
                        if (begin_custom_level(editorPath, levelError)) {
                            prevNs = SDL_GetTicksNS();
                            accNs = 0;
                        } else {
                            levelErrorPath = editorPath;
                            screen = Screen::LEVEL_LOAD_ERROR;
                        }
                    }
                    break;
                }
                case Screen::LEVEL_EDITOR_TEXT_INPUT: {
                    if (ev.key.key == SDLK_BACKSPACE) {
                        if (!editorTextBuf.empty()) editorTextBuf.pop_back();
                    } else if (ev.key.key == SDLK_RETURN
                            || ev.key.key == SDLK_KP_ENTER) {
                        apply_editor_text();
                    } else if (ev.key.key == SDLK_ESCAPE) {
                        editorTextField = LevelEditorTextField::NONE;
                        editorTextError.clear();
                        stop_text_input();
                        screen = Screen::LEVEL_EDITOR;
                    }
                    break;
                }
                case Screen::LEVEL_LOAD_ERROR: {
                    if (ev.key.key == SDLK_ESCAPE
                        || ev.key.key == SDLK_RETURN
                        || ev.key.key == SDLK_KP_ENTER) {
                        refresh_level_browser();
                        screen = Screen::CUSTOM_LEVELS;
                    }
                    break;
                }
                case Screen::REPLAY_BROWSER: {
                    auto act = replayBrowserMenu.handle_key(ev.key.key);
                    if (act == MenuList::Action::ACCEPT) {
                        const int tag = replayBrowserMenu.selected_tag();
                        if (tag >= 0
                            && tag < static_cast<int>(replayChoices.size())) {
                            if (begin_replay(replayChoices[tag].path, replayError)) {
                                prevNs = SDL_GetTicksNS();
                                accNs = 0;
                            }
                        } else if (tag == REPLAY_BROWSER_MANUAL) {
                            replayPathBuf.clear();
                            replayError.clear();
                            start_text_input();
                            screen = Screen::REPLAY_INPUT;
                        } else if (tag == FILE_BROWSER_BACK) {
                            replayError.clear();
                            screen = Screen::MAIN_MENU;
                        }
                    } else if (act == MenuList::Action::CANCEL) {
                        replayError.clear();
                        screen = Screen::MAIN_MENU;
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
                case Screen::REPLAY_INPUT: {
                    if (ev.key.key == SDLK_BACKSPACE) {
                        if (!replayPathBuf.empty()) replayPathBuf.pop_back();
                    } else if (ev.key.key == SDLK_RETURN
                            || ev.key.key == SDLK_KP_ENTER) {
                        if (begin_replay(replayPathBuf, replayError)) {
                            stop_text_input();
                            prevNs = SDL_GetTicksNS();
                            accNs = 0;
                        }
                    } else if (ev.key.key == SDLK_ESCAPE) {
                        stop_text_input();
                        replayError.clear();
                        refresh_replay_browser();
                        screen = Screen::REPLAY_BROWSER;
                    }
                    break;
                }
                case Screen::SETTINGS: {

                    auto act = settingsMenu.handle_key(ev.key.key);
                    if (act == MenuList::Action::ACCEPT) {
                        int tag = settingsMenu.selected_tag();
                        if (tag == -1) {

                            audio.set_muted(cfg.sdl3_muted);
                            director.set_enabled(cfg.sdl3_director_enabled);
                            gfx.set_reduced_motion(cfg.sdl3_reduced_motion);
                            persist_all();
                            screen = Screen::MAIN_MENU;
                            break;
                        }
                        adjust_setting(cfg, tag, +1);
                        gfx.set_reduced_motion(cfg.sdl3_reduced_motion);
                        settingsMenu.set_items(make_settings_menu(cfg));
                    } else if (act == MenuList::Action::CANCEL) {
                        audio.set_muted(cfg.sdl3_muted);
                        director.set_enabled(cfg.sdl3_director_enabled);
                        gfx.set_reduced_motion(cfg.sdl3_reduced_motion);
                        persist_all();
                        screen = Screen::MAIN_MENU;
                    } else {
                        if (ev.key.key == SDLK_LEFT) {
                            adjust_setting(cfg, settingsMenu.selected_tag(), -1);
                            gfx.set_reduced_motion(cfg.sdl3_reduced_motion);
                            settingsMenu.set_items(make_settings_menu(cfg));
                        } else if (ev.key.key == SDLK_RIGHT) {
                            adjust_setting(cfg, settingsMenu.selected_tag(), +1);
                            gfx.set_reduced_motion(cfg.sdl3_reduced_motion);
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

                        save_last_replay_if_needed();
                        submit_leaderboard_if_good();
                        persist_all();
                        refresh_main_menu();
                        if (coopActive) clear_coop_connection();
                        screen = Screen::MAIN_MENU;
                    } else if (ev.key.key == SDLK_P) {

                        pauseMenu.set_items(make_pause_menu());
                        screen = Screen::PAUSED;
                    } else {

                        if (auto* kbd = dynamic_cast<SDL3Keyboard*>(inputP1.get())) {
                            kbd->note_key_down(ev.key.key);
                        } else if (coopKeyboard) {
                            coopKeyboard->note_key_down(ev.key.key);
                        }
                    }
                    break;
                }
                case Screen::PAUSED: {
                    auto act = pauseMenu.handle_key(ev.key.key);
                    if (act == MenuList::Action::ACCEPT) {
                        switch (pauseMenu.selected_tag()) {
                        case 0:
                            screen = Screen::PLAYING;

                            prevNs = SDL_GetTicksNS();
                            accNs  = 0;
                            break;
                        case 1: {
                            save_last_replay_if_needed();
                            if (replayActive) {
                                begin_replay(replayPath, replayError);
                            } else if (customLevelActive) {
                                begin_custom_level(customLevelPath, levelError);
                            } else {
                                begin_play(cfg.default_diff,
                                           aiDemoActive ? Mode::AI_DEMO : Mode::SOLO,
                                           time_seed(), false, nullptr);
                            }
                            prevNs = SDL_GetTicksNS();
                            accNs  = 0;
                            break;
                        }
                        case 2:
                            save_last_replay_if_needed();
                            submit_leaderboard_if_good();
                            persist_all();
                            refresh_main_menu();
                            if (coopActive) clear_coop_connection();
                            screen = Screen::MAIN_MENU;
                            break;
                        }
                    } else if (act == MenuList::Action::CANCEL
                            || ev.key.key == SDLK_P) {

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
                        case 0:
                            save_last_replay_if_needed();
                            submit_leaderboard_if_good();
                            persist_all();
                            if (coopActive) {
                                clear_coop_connection();
                                coopMenu.set_items(make_coop_menu());
                                screen = Screen::COOP_MENU;
                            } else if (replayActive) {
                                begin_replay(replayPath, replayError);
                            } else if (customLevelActive) {
                                begin_custom_level(customLevelPath, levelError);
                            } else {
                                begin_play(cfg.default_diff,
                                           aiDemoActive ? Mode::AI_DEMO : Mode::SOLO,
                                           time_seed(), false, nullptr);
                            }
                            prevNs = SDL_GetTicksNS();
                            accNs = 0;
                            break;
                        case 1:
                            save_last_replay_if_needed();
                            submit_leaderboard_if_good();
                            persist_all();
                            refresh_main_menu();
                            if (coopActive) clear_coop_connection();
                            screen = Screen::MAIN_MENU;
                            break;
                        case 2:
                            save_last_replay_if_needed();
                            submit_leaderboard_if_good();
                            persist_all();
                            if (coopActive) clear_coop_connection();
                            quitRequested = true;
                            break;
                        }
                    } else if (ev.key.key == SDLK_R) {
                        save_last_replay_if_needed();
                        submit_leaderboard_if_good();
                        persist_all();
                        if (coopActive) {
                            clear_coop_connection();
                            coopMenu.set_items(make_coop_menu());
                            screen = Screen::COOP_MENU;
                        } else if (replayActive) {
                            begin_replay(replayPath, replayError);
                        } else if (customLevelActive) {
                            begin_custom_level(customLevelPath, levelError);
                        } else {
                            begin_play(cfg.default_diff,
                                       aiDemoActive ? Mode::AI_DEMO : Mode::SOLO,
                                       time_seed(), false, nullptr);
                        }
                        prevNs = SDL_GetTicksNS();
                        accNs = 0;
                    } else if (act == MenuList::Action::CANCEL) {
                        save_last_replay_if_needed();
                        submit_leaderboard_if_good();
                        persist_all();
                        refresh_main_menu();
                        if (coopActive) clear_coop_connection();
                        screen = Screen::MAIN_MENU;
                    }
                    break;
                }
                case Screen::REPLAY_SUMMARY: {
                    auto restart_replay = [&]() {
                        if (begin_replay(replayPath, replayError)) {
                            prevNs = SDL_GetTicksNS();
                            accNs = 0;
                        } else {
                            refresh_replay_browser();
                            screen = Screen::REPLAY_BROWSER;
                        }
                    };

                    auto return_to_menu = [&]() {
                        clear_finished_replay();
                        refresh_main_menu();
                        screen = Screen::MAIN_MENU;
                    };

                    auto act = gameoverMenu.handle_key(ev.key.key);
                    if (act == MenuList::Action::ACCEPT) {
                        switch (gameoverMenu.selected_tag()) {
                        case 0:
                            restart_replay();
                            break;
                        case 1:
                            return_to_menu();
                            break;
                        case 2:
                            clear_finished_replay();
                            quitRequested = true;
                            break;
                        }
                    } else if (ev.key.key == SDLK_R) {
                        restart_replay();
                    } else if (act == MenuList::Action::CANCEL) {
                        return_to_menu();
                    }
                    break;
                }
                default: break;
                }
            }
            if (ev.type == SDL_EVENT_TEXT_INPUT) {
                if (screen == Screen::USERNAME_INPUT) {

                    for (const char* p = ev.text.text; *p && typingBuf.size() < 16; ++p) {
                        if (*p >= 0x20 && *p < 0x7f && *p != ' ') {
                            typingBuf.push_back(*p);
                        }
                    }
                } else if (screen == Screen::COOP_JOIN_INPUT) {
                    coopError.clear();
                    for (const char* p = ev.text.text; *p && coopJoinBuf.size() < 45; ++p) {
                        const bool ok = (*p >= '0' && *p <= '9')
                                     || *p == '.'
                                     || *p == ':'
                                     || (*p >= 'a' && *p <= 'z')
                                     || (*p >= 'A' && *p <= 'Z')
                                     || *p == '-';
                        if (ok) coopJoinBuf.push_back(*p);
                    }
                } else if (screen == Screen::LEVEL_EDITOR_TEXT_INPUT) {
                    editorTextError.clear();
                    for (const char* p = ev.text.text; *p && editorTextBuf.size() < 96; ++p) {
                        if (*p >= 0x20 && *p < 0x7f) {
                            editorTextBuf.push_back(*p);
                        }
                    }
                } else if (screen == Screen::REPLAY_INPUT) {
                    replayError.clear();
                    for (const char* p = ev.text.text; *p && replayPathBuf.size() < 96; ++p) {
                        if (*p >= 0x20 && *p < 0x7f) {
                            replayPathBuf.push_back(*p);
                        }
                    }
                }
            }
        }
        if (quitRequested) break;

        if (screen == Screen::COOP_CONNECTING && coopFuture.valid()) {
            const auto ready = coopFuture.wait_for(std::chrono::milliseconds(0));
            if (ready == std::future_status::ready) {
                CoopConnectResult result = coopFuture.get();
                if (result.ok) {
                    begin_coop_game(std::move(result));
                    prevNs = SDL_GetTicksNS();
                    accNs = 0;
                } else {
                    coopError = result.error.empty()
                        ? "Connection failed."
                        : result.error;
                    screen = Screen::COOP_ERROR;
                }
            }
        }

        if (screen != Screen::PLAYING) {
            audio.set_music(AudioSystem::Music::NONE, 0.0f);
        }

        Uint64 nowNs = SDL_GetTicksNS();
        Uint64 dtNs  = nowNs - prevNs;
        prevNs = nowNs;

        if (screen == Screen::PLAYING && !game->is_game_over()) {
            accNs += dtNs;
            if (accNs > maxAccNs) accNs = maxAccNs;
            while (accNs >= tickNs && !game->is_game_over()) {
                accNs -= tickNs;
                gfx.pre_step(*game);
                const std::uint32_t simTick = game->tick();
                std::uint8_t m1 = inputP1 ? inputP1->poll(simTick, *game, 0) : 0;
                std::uint8_t m2 = inputP2 ? inputP2->poll(simTick, *game, 1) : 0;
                game->step_pub(m1, m2);
                gfx.post_step(*game);
                audio.observe(*game);

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

                if (!replayActive) {

                    const float tickSec = static_cast<float>(FRAME_MS) / 1000.0f;
                    director.observe(*game, tickSec);
                    const auto m = director.modifiers();
                    game->set_director_modifiers(m.shootMul, m.moveMul, m.dropMul);
                } else {
                    game->set_director_modifiers(1.0f, 1.0f, 1.0f);
                }

                game->tick_flash_decay();
                if (coopActive && coopDead.load()) {
                    save_last_replay_if_needed();
                    submit_leaderboard_if_good();
                    persist_all();
                    clear_coop_connection();
                    coopError = "Peer disconnected.";
                    screen = Screen::COOP_ERROR;
                    break;
                }
                if (game->quit_flag()) {
                    save_last_replay_if_needed();
                    submit_leaderboard_if_good();
                    persist_all();
                    refresh_main_menu();
                    if (coopActive) clear_coop_connection();
                    audio.set_music(AudioSystem::Music::NONE, 0.0f);
                    screen = Screen::MAIN_MENU;
                    break;
                }
            }
            if (screen == Screen::PLAYING && game->is_game_over()) {
                save_last_replay_if_needed();
                if (replayActive) {
                    replaySummary = build_replay_summary();
                    gameoverMenu.set_items(make_replay_over_menu());
                    screen = Screen::REPLAY_SUMMARY;
                } else {
                    gameoverMenu.set_items(make_gameover_menu());
                    screen = Screen::GAME_OVER;
                }
            }
        }

        const float dtSec = static_cast<float>(dtNs) / 1.0e9f;
        gfx.tick_render(dtSec);

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
        case Screen::COOP_MENU: {
            draw_coop_menu(renderer, coopMenu, cfg.net_port);
            break;
        }
        case Screen::COOP_JOIN_INPUT: {
            Uint64 elapsed = SDL_GetTicksNS() - cursorBlinkStart;
            bool   blinkOn = ((elapsed / 500'000'000ull) % 2) == 0;
            draw_coop_join_input(renderer, coopJoinBuf, coopError, blinkOn);
            break;
        }
        case Screen::COOP_CONNECTING: {
            draw_coop_connecting(renderer, coopStatus);
            break;
        }
        case Screen::COOP_ERROR: {
            draw_coop_error(renderer, coopError);
            break;
        }
        case Screen::DIFFICULTY_SELECT: {
            draw_difficulty_select(renderer, difficultyMenu);
            break;
        }
        case Screen::CUSTOM_LEVELS: {
            draw_custom_levels(renderer, levelBrowserMenu, levelError,
                               !levelChoices.empty());
            break;
        }
        case Screen::LEVEL_PREVIEW: {
            draw_level_preview(renderer, levelPreview,
                               levelPreviewPath, levelPreviewMenu);
            break;
        }
        case Screen::LEVEL_EDITOR: {
            draw_level_editor(renderer, editorLevel, editorPath,
                              editorGrid,
                              editorAlienRow, editorAlienCol,
                              editorShieldRow, editorShieldCol,
                              editorMessage);
            break;
        }
        case Screen::LEVEL_EDITOR_TEXT_INPUT: {
            Uint64 elapsed = SDL_GetTicksNS() - cursorBlinkStart;
            bool   blinkOn = ((elapsed / 500'000'000ull) % 2) == 0;
            draw_level_editor_text_input(renderer, editorTextLabel,
                                         editorTextBuf,
                                         editorTextError,
                                         blinkOn);
            break;
        }
        case Screen::LEVEL_LOAD_ERROR: {
            draw_level_load_error(renderer, levelErrorPath, levelError);
            break;
        }
        case Screen::REPLAY_BROWSER: {
            draw_replay_browser(renderer, replayBrowserMenu, replayError,
                                !replayChoices.empty());
            break;
        }
        case Screen::REPLAY_INPUT: {
            Uint64 elapsed = SDL_GetTicksNS() - cursorBlinkStart;
            bool   blinkOn = ((elapsed / 500'000'000ull) % 2) == 0;
            draw_replay_input(renderer, replayPathBuf, replayError, blinkOn);
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
            if (replayActive) draw_replay_hud(renderer, replayData, replayPath);
            else              draw_director_hud(renderer, director);
            break;
        }
        case Screen::PAUSED: {

            gfx.draw(renderer, *game, 1.0f);
            if (replayActive) draw_replay_hud(renderer, replayData, replayPath);
            else              draw_director_hud(renderer, director);
            draw_pause_overlay(renderer, pauseMenu, *game);
            break;
        }
        case Screen::GAME_OVER: {
            gfx.draw(renderer, *game, 1.0f);
            if (replayActive) draw_replay_hud(renderer, replayData, replayPath);
            else              draw_director_hud(renderer, director);
            const bool isNewBest = !replayActive && (game->score() > preGameBest);
            draw_game_over(renderer, gameoverMenu, *game, isNewBest);
            break;
        }
        case Screen::REPLAY_SUMMARY: {
            if (game) {
                gfx.draw(renderer, *game, 1.0f);
                draw_replay_hud(renderer, replayData, replayPath);
            }
            draw_replay_summary(renderer, replaySummary, gameoverMenu);
            break;
        }
        default: break;
        }

        SDL_RenderPresent(renderer);
    }

    save_last_replay_if_needed();
    submit_leaderboard_if_good();
    persist_all();

    audio.shutdown();
    if (coopFuture.valid()) coopFuture.wait();
    clear_coop_connection();
    platform::net_cleanup();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

}

int main(int argc, char** argv) {
    return si::main_sdl3(argc, argv);
}
