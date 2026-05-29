// menus.cpp
#include "menus.h"
#include "banner.h"

#include "../core/action.h"
#include "../core/colors.h"
#include "../core/difficulty.h"
#include "../debug/logger.h"
#include "../editor/level_editor.h"
#include "../game/game.h"
#include "../i18n/strings.h"
#include "../input/ai_source.h"
#include "../input/coop_source.h"
#include "../input/input_source.h"
#include "../input/keyboard_source.h"
#include "../input/replay_source.h"
#include "../net/tcp_socket.h"
#include "../persistence/replay_file.h"
#include "../platform/platform.h"
#include "../ui/ui_options.h"

#include <atomic>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

namespace si {

// ---- helpers ----

static std::uint32_t pick_seed(std::uint32_t override) {
    return (override != 0) ? override : (std::uint32_t)std::time(nullptr);
}

static void post_game(Game& game, const SaveState& res,
                      Record& rec, const std::string& user,
                      Stats& stats, std::vector<Achievement>& ach,
                      SaveState& saved) {
    std::cout << "\n  Press ENTER to continue...";
    std::cin.get();
    save_delete(user);
    saved.valid = false;

    if (res.score > rec.score) {
        rec.score = res.score;
        rec.level = res.level;
        rec.diff  = difficulty_unchecked(res.diffIdx).name;
        record_write(user, rec);
        leaderboard_submit({ user, rec.score, rec.level, rec.diff });
        std::cout << color::BGREEN
                  << "\n  *** NEW BEST: " << rec.score << " pts! ***\n"
                  << color::RST;
        platform::sleep_ms(1500);
    }
    stats_write(user, stats);
    achievements_write(user, ach);

    if (!game.replay().frames.empty()) {
        std::cout << "\n  Save replay to file? Enter filename (blank to skip): ";
        std::string fn; std::getline(std::cin, fn);
        if (!fn.empty()) {
            if (fn.find('.') == std::string::npos) fn += ".rpl";
            Replay rp = game.replay();
            rp.player        = user;
            rp.expectedScore = res.score;
            rp.expectedLevel = res.level;
            if (replay_save(fn, rp))
                std::cout << color::BGREEN << "  Replay saved to " << fn
                          << '\n' << color::RST;
            platform::sleep_ms(700);
        }
    }
}

// ---- solo ----

void run_solo(int diffIdx, const std::string& user, Record& rec,
              SaveState& saved, Stats& stats,
              std::vector<Achievement>& ach) {
    auto seed = pick_seed(0);
    InputState inp;
    std::thread thr(input_thread_main, std::ref(inp));
    KeyboardSource k(inp);
    Game game(diffIdx, Mode::SOLO, seed, stats, ach);
    game.set_telemetry_user(user);
    SaveState res = game.run(&k, nullptr);
    inp.running = false;
    thr.join();

    if (game.quit_flag() && !difficulty_unchecked(diffIdx).oneLife) {
        saved = game.snap();
        save_write(user, saved);
    } else {
        // Auto-save the last run so it can always be replayed.
        if (!game.replay().frames.empty()) {
            Replay rp = game.replay();
            rp.player        = user;
            rp.expectedScore = res.score;
            rp.expectedLevel = res.level;
            replay_save(user + "_last.rpl", rp);
        }
        post_game(game, res, rec, user, stats, ach, saved);
    }
}

void run_from_save(const SaveState& s, const std::string& user, Record& rec,
                   SaveState& saved, Stats& stats,
                   std::vector<Achievement>& ach) {
    InputState inp;
    std::thread thr(input_thread_main, std::ref(inp));
    KeyboardSource k(inp);
    Game game(s, stats, ach);
    game.set_telemetry_user(user);
    SaveState res = game.run(&k, nullptr);
    inp.running = false;
    thr.join();

    if (game.quit_flag()) {
        saved = game.snap();
        save_write(user, saved);
    } else {
        post_game(game, res, rec, user, stats, ach, saved);
    }
}

// ---- AI demo ----
// A small multiplexer source that mostly returns AI output, but lets
// the human press Q to exit.
namespace {
struct AiPlusQuit : IInputSource {
    AISource&        ai;
    KeyboardSource&  kb;
    AiPlusQuit(AISource& a, KeyboardSource& k) : ai(a), kb(k) {}
    std::uint8_t poll(std::uint32_t t, const Game& g, int p) override {
        std::uint8_t a = ai.poll(t, g, p);
        std::uint8_t k = kb.poll(t, g, p);
        return a | (k & action::QUIT);
    }
};
}

void run_ai_demo(int diffIdx, const std::string& user,
                 Stats& stats, std::vector<Achievement>& ach,
                 const std::string& ai_profile, std::uint32_t seed) {
    std::uint32_t s = pick_seed(seed);
    LOG_INFO("ai_demo: seed=" << s << " profile=" << ai_profile);

    InputState inp;
    std::thread thr(input_thread_main, std::ref(inp));
    AISource ai(ai_profile_by_name(ai_profile));
    KeyboardSource kb(inp);
    AiPlusQuit mux(ai, kb);

    Game game(diffIdx, Mode::AI_DEMO, s, stats, ach);
    game.run(&mux, nullptr);
    inp.running = false;
    thr.join();
    stats_write(user, stats);
    achievements_write(user, ach);

    if (!game.replay().frames.empty()) {
        std::cout << "\n  Save AI replay to file? Enter filename (blank to skip): ";
        std::string fn; std::getline(std::cin, fn);
        if (!fn.empty()) {
            if (fn.find('.') == std::string::npos) fn += ".rpl";
            Replay rp = game.replay();
            rp.player        = user + "_AI_" + ai_profile;
            rp.expectedScore = game.score();
            rp.expectedLevel = game.level();
            if (replay_save(fn, rp))
                std::cout << color::BGREEN << "  Replay saved to " << fn
                          << '\n' << color::RST;
            platform::sleep_ms(700);
        }
    }
}

// ---- replay ----

void run_replay(const std::string& path, Stats& stats,
                std::vector<Achievement>& ach, const std::string& user) {
    Replay rp;
    if (!replay_load(path, rp)) {
        std::cout << color::BRED << "  Could not load " << path << '\n' << color::RST;
        platform::sleep_ms(1200);
        return;
    }
    std::cout << color::BCYAN << "  Replaying " << path
              << " by " << rp.player
              << " on " << difficulty_unchecked(rp.diffIdx).name
              << "\n  Press ENTER to begin..." << color::RST;
    std::cin.get();
    ReplaySource r1(rp.frames, 1), r2(rp.frames, 2);
    Game game(rp.diffIdx, Mode::REPLAY, rp.seed, stats, ach);
    game.run(&r1, &r2);
    std::cout << "\n  Press ENTER to continue..."; std::cin.get();
    (void)user;
}

// ---- network co-op ----

void run_host(int diffIdx, const std::string& user,
              Stats& stats, std::vector<Achievement>& ach,
              int net_port) {
    Logger::get().set_tag("HOST");
    LOG_INFO("hosting on port " << net_port);

    std::cout << "\n  Hosting on port " << net_port
              << " ... waiting for client to connect.\n";
    net::TCPSocket sock = net::net_host(net_port);
    if (!sock.valid()) {
        std::cout << color::BRED << "  Failed to host (port busy?).\n" << color::RST;
        platform::sleep_ms(1500);
        return;
    }
    auto seed = pick_seed(0);
    std::stringstream ss;
    ss << "HELLO " << seed << ' ' << diffIdx;
    if (!sock.sendLine(ss.str())) {
        std::cout << color::BRED << "  Handshake send failed.\n" << color::RST;
        platform::sleep_ms(1500);
        return;
    }
    std::string reply;
    if (!sock.recvLine(reply) || reply != "OK") {
        std::cout << color::BRED << "  Handshake failed.\n" << color::RST;
        platform::sleep_ms(1500);
        return;
    }
    std::cout << color::BGREEN << "  Peer connected! Starting in 2s...\n" << color::RST;
    platform::sleep_ms(1500);

    InputState inp;
    std::thread thr(input_thread_main, std::ref(inp));
    KeyboardSource kb(inp);
    std::atomic<bool> dead{ false };
    CoopSource cs(sock, kb, dead, /*self_player=*/0);

    Game game(diffIdx, Mode::COOP_HOST, seed, stats, ach);
    game.run(&cs, &cs, &dead);
    inp.running = false;
    thr.join();
    stats_write(user, stats);
    achievements_write(user, ach);
    std::cout << "\n  Press ENTER to continue..."; std::cin.get();
    Logger::get().set_tag("");
}

void run_join(const std::string& user, const std::string& ip,
              Stats& stats, std::vector<Achievement>& ach,
              int net_port) {
    Logger::get().set_tag("CLIENT");
    LOG_INFO("joining " << ip << ":" << net_port);

    net::TCPSocket sock = net::net_join(ip, net_port);
    if (!sock.valid()) {
        std::cout << color::BRED << "  Could not connect to " << ip
                  << '\n' << color::RST;
        platform::sleep_ms(1500);
        return;
    }
    std::string hello;
    if (!sock.recvLine(hello) || hello.rfind("HELLO ", 0) != 0) {
        std::cout << color::BRED << "  Bad handshake.\n" << color::RST;
        platform::sleep_ms(1500);
        return;
    }
    std::stringstream pp(hello);
    std::string tag; std::uint32_t seed; int diffIdx;
    pp >> tag >> seed >> diffIdx;
    if (!sock.sendLine("OK")) {
        std::cout << color::BRED << "  Send OK failed.\n" << color::RST;
        platform::sleep_ms(1500);
        return;
    }
    std::cout << color::BGREEN << "  Connected. Difficulty "
              << difficulty_unchecked(diffIdx).name
              << ". Starting in 2s...\n" << color::RST;
    platform::sleep_ms(1500);

    InputState inp;
    std::thread thr(input_thread_main, std::ref(inp));
    KeyboardSource kb(inp);
    std::atomic<bool> dead{ false };
    CoopSource cs(sock, kb, dead, /*self_player=*/1);

    Game game(diffIdx, Mode::COOP_CLIENT, seed, stats, ach);
    game.run(&cs, &cs, &dead);
    inp.running = false;
    thr.join();
    stats_write(user, stats);
    achievements_write(user, ach);
    std::cout << "\n  Press ENTER to continue..."; std::cin.get();
    Logger::get().set_tag("");
}

// ---- AI training (headless) ----

int run_train_ai(int n, int diffIdx, const std::string& ai_profile) {
    std::ofstream f("ai_train.csv");
    f << "run,score,level,profile,seed\n";
    Stats st; auto ach = achievements_default();
    AIProfile prof = ai_profile_by_name(ai_profile);

    std::cout << color::BCYAN << "  Training AI: " << n << " runs, profile="
              << prof.name << ", diff=" << difficulty_unchecked(diffIdx).name
              << color::RST << "\n\n";

    int total = 0, best = 0;
    for (int i = 1; i <= n; ++i) {
        std::uint32_t seed = (std::uint32_t)std::time(nullptr) + i * 1009u;
        AISource ai(prof);
        Game game(diffIdx, Mode::AI_DEMO, seed, st, ach);
        SaveState r = game.run_headless(&ai, nullptr, /*max_ticks=*/6000);
        (void)r;
        int sc = game.score(), lv = game.level();
        f << i << ',' << sc << ',' << lv << ',' << prof.name << ',' << seed << '\n';
        f.flush();
        total += sc;
        if (sc > best) best = sc;
        std::cout << "  run " << std::setw(3) << i
                  << ": score=" << std::setw(5) << sc
                  << "  level=" << lv << '\n';
    }
    std::cout << color::BGREEN
              << "\n  Done. Avg score = " << (total / std::max(1, n))
              << ", best = " << best
              << ".  CSV written to ai_train.csv\n" << color::RST;
    return 0;
}

// ---- settings (interactive, in-game) ----

void show_settings(Config& cfg) {
    using i18n::tr;
    auto on_off = [](bool b) { return b ? "ON" : "off"; };
    while (true) {
        (void)!std::system(SI_CLEAR_CMD);
        std::cout << color::BOLD << color::BCYAN
                  << "\n  +==========================================+\n"
                  <<   "  |              " << tr("settings.title")
                                            << "              |\n"
                  <<   "  +==========================================+\n"
                  << color::RST
                  << "    [1] " << tr("settings.colorblind") << "  : "
                  << color::BWHITE << on_off(cfg.colorblind) << color::RST << '\n'
                  << "    [2] " << tr("settings.sound")      << "  : "
                  << color::BWHITE << on_off(cfg.sound)      << color::RST << '\n'
                  << "    [3] " << tr("settings.ai_profile") << "      : "
                  << color::BWHITE << cfg.ai_profile         << color::RST << '\n'
                  << "    [4] " << tr("settings.language")   << "        : "
                  << color::BWHITE << cfg.language           << color::RST << '\n'
                  << "    [5] Quick-restart at game over : "
                  << color::BWHITE << on_off(cfg.quick_restart) << color::RST << '\n'
                  << "    [6] Default difficulty         : "
                  << color::BWHITE << (cfg.default_diff + 1)
                  << " (" << difficulty_unchecked(cfg.default_diff).name << ")"
                  << color::RST << '\n'
                  << "    [7] Network port              : "
                  << color::BWHITE << cfg.net_port << color::RST << '\n'
                  << "    [8] Log level                 : "
                  << color::BWHITE << cfg.log_level << color::RST << '\n'
                  << '\n'
                  << "    " << tr("settings.back") << "    [W] Write changes to si_pro.cfg\n\n"
                  << "  Choice: ";
        char ch; std::cin >> ch; std::cin.ignore(10000, '\n');
        switch (ch) {
            case '1':
                cfg.colorblind = !cfg.colorblind;
                ui::opts().colorblind = cfg.colorblind;
                break;
            case '2':
                cfg.sound = !cfg.sound;
                ui::opts().sound = cfg.sound;
                if (cfg.sound) std::cout << '\a';   // confirmation beep
                break;
            case '3': {
                // rotate balanced -> aggressive -> defensive -> balanced
                if      (cfg.ai_profile == "balanced")   cfg.ai_profile = "aggressive";
                else if (cfg.ai_profile == "aggressive") cfg.ai_profile = "defensive";
                else                                      cfg.ai_profile = "balanced";
                break;
            }
            case '4': {
                cfg.language = (cfg.language == "en") ? "hi" : "en";
                i18n::set_language(cfg.language);
                break;
            }
            case '5':
                cfg.quick_restart = !cfg.quick_restart;
                break;
            case '6':
                cfg.default_diff = (cfg.default_diff + 1) % N_DIFFS;
                break;
            case '7': {
                std::cout << "  New port (1024-65535): ";
                int p; std::cin >> p; std::cin.ignore(10000, '\n');
                if (p >= 1024 && p <= 65535) cfg.net_port = p;
                break;
            }
            case '8': {
                if      (cfg.log_level == "off")   cfg.log_level = "info";
                else if (cfg.log_level == "info")  cfg.log_level = "debug";
                else if (cfg.log_level == "debug") cfg.log_level = "warn";
                else if (cfg.log_level == "warn")  cfg.log_level = "error";
                else                                cfg.log_level = "off";
                break;
            }
            case 'w': case 'W':
                if (save_config("si_pro.cfg", cfg))
                    std::cout << color::BGREEN
                              << "  Wrote si_pro.cfg\n" << color::RST;
                else
                    std::cout << color::BRED
                              << "  Could not write si_pro.cfg\n" << color::RST;
                platform::sleep_ms(900);
                break;
            case 'b': case 'B':
                return;
            default:
                std::cout << "  ?\n";
                platform::sleep_ms(400);
        }
    }
}

// ---- credits screen ----

void show_credits() {
    using i18n::tr;
    (void)!std::system(SI_CLEAR_CMD);
    std::cout << color::BOLD << color::BMAGENTA
              << "\n  +==========================================+\n"
              <<   "  |               " << tr("credits.title")
                                          << "                |\n"
              <<   "  +==========================================+\n"
              << color::RST;
    std::cout << "\n"
              << "    Space Invaders - Pro Edition\n"
              << "    Final-year B.E. (CSE) project\n"
              << "    Chitkara University Himachal Pradesh\n"
              << "    Academic year 2025-26\n\n"
              << "    Author      : Anurag Yadav\n"
              << "    Student ID  : 2211981086\n"
              << "    Language    : Modern C++17\n"
              << "    LoC         : ~4500 across 70+ source files\n\n"
              << "    Built with help from the C++ standard library,\n"
              << "    learncpp.com, and a lot of debugging.\n\n"
              << "    The original 1978 arcade game is by Tomohiro Nishikado\n"
              << "    at Taito Corporation. This is an homage, not a copy.\n";
    std::cout << "\n\n  Press ENTER..."; std::cin.get();
}

// ---- main menu ----

void run_menu(const Config& cfg, const std::string& user, Record& rec,
              SaveState& saved, Stats& stats,
              std::vector<Achievement>& ach) {
    using i18n::tr;
    // Take a mutable copy so the settings screen can edit values
    // without surprising the rest of the program. Persistent changes
    // are written back to si_pro.cfg from the settings screen.
    Config live_cfg = cfg;
    while (true) {
        banner();
        std::cout << "  " << tr("menu.welcome") << ' '
                  << color::BWHITE << user << color::RST << "!\n\n";
        show_record(rec);
        std::cout << '\n' << color::BWHITE
                  << "  +---------------------------------+\n"
                  << "  |           " << tr("menu.title") << "            |\n"
                  << "  +---------------------------------+\n"
                  << "  |  " << tr("menu.new_game") << "           |\n";
        if (saved.valid)
            std::cout << "  |  " << tr("menu.continue_save")
                      << "  (Lv " << std::setw(2) << saved.level
                      << " Sc " << std::setw(5) << saved.score << ") |\n";
        else
            std::cout << "  |  " << tr("menu.continue_none") << "       |\n";
        std::cout << "  |  " << tr("menu.ai_demo")     << " |\n"
                  << "  |  " << tr("menu.host")        << "         |\n"
                  << "  |  " << tr("menu.join")        << "         |\n"
                  << "  |  " << tr("menu.replay")      << "       |\n"
                  << "  |  " << tr("menu.editor")      << "              |\n"
                  << "  |  " << tr("menu.leaderboard") << "               |\n"
                  << "  |  " << tr("menu.stats")       << " |\n"
                  << "  |  " << tr("menu.settings")    << "                  |\n"
                  << "  |  " << tr("menu.credits")     << "                   |\n"
                  << "  |  " << tr("menu.quit")        << "                      |\n"
                  << "  +---------------------------------+\n" << color::RST
                  << "\n  " << tr("menu.choice");

        char ch; std::cin >> ch; std::cin.ignore(10000, '\n');

        switch (ch) {
            case '0':
                std::cout << "\n  " << tr("menu.farewell") << ' '
                          << user << ".\n\n";
                return;

            case '1': {
                int di = pick_difficulty();
                if (di < 0) { platform::sleep_ms(500); continue; }
                if (saved.valid) {
                    std::cout << "\n  Existing save (Lv " << saved.level
                              << ") will be lost. Continue? [Y/N]: ";
                    char yn; std::cin >> yn; std::cin.ignore(10000, '\n');
                    if (yn != 'y' && yn != 'Y') continue;
                    save_delete(user); saved.valid = false;
                }
                run_solo(di, user, rec, saved, stats, ach);
                // Quick-restart loop: keep launching solo runs at the same
                // difficulty until the user chooses NOT to retry. This is
                // gated by a config flag so it doesn't surprise anyone.
                while (live_cfg.quick_restart && !saved.valid) {
                    std::cout << "\n  " << color::BCYAN
                              << "Press R to retry "
                              << difficulty_unchecked(di).name
                              << ", ENTER for menu: "
                              << color::RST;
                    std::string line; std::getline(std::cin, line);
                    if (line != "r" && line != "R") break;
                    run_solo(di, user, rec, saved, stats, ach);
                }
                break;
            }
            case '2': {
                if (!saved.valid) {
                    std::cout << "\n  No save found.\n";
                    platform::sleep_ms(900); continue;
                }
                if (difficulty_unchecked(saved.diffIdx).oneLife) {
                    std::cout << "\n  Ultra-Nightmare cannot be resumed. Save cleared.\n";
                    save_delete(user); saved.valid = false;
                    platform::sleep_ms(1200); continue;
                }
                run_from_save(saved, user, rec, saved, stats, ach);
                break;
            }
            case '3': {
                int di = pick_difficulty();
                if (di < 0) { platform::sleep_ms(500); continue; }
                std::cout << color::BBLUE
                          << "\n  AI bot will now play (profile=" << live_cfg.ai_profile
                          << "). Press Q to stop.\n" << color::RST;
                platform::sleep_ms(1000);
                run_ai_demo(di, user, stats, ach, live_cfg.ai_profile, live_cfg.ai_seed);
                break;
            }
            case '4': {
                int di = pick_difficulty();
                if (di < 0) { platform::sleep_ms(500); continue; }
                run_host(di, user, stats, ach, live_cfg.net_port);
                break;
            }
            case '5': {
                std::cout << "\n  " << tr("prompt.host_ip");
                std::string ip; std::getline(std::cin, ip);
                if (ip.empty()) ip = "127.0.0.1";
                run_join(user, ip, stats, ach, live_cfg.net_port);
                break;
            }
            case '6': {
                std::cout << "\n  " << tr("prompt.replay_file");
                std::string fn; std::getline(std::cin, fn);
                if (!fn.empty()) {
                    if (fn.find('.') == std::string::npos) fn += ".rpl";
                    run_replay(fn, stats, ach, user);
                }
                break;
            }
            case '7':
                run_level_editor(user);
                break;
            case '8':
                show_leaderboard();
                break;
            case '9':
                show_stats(stats);
                show_achievements(ach);
                break;
            case 's': case 'S':
                show_settings(live_cfg);
                break;
            case 'c': case 'C':
                show_credits();
                break;
            default:
                std::cout << "\n  " << tr("menu.invalid") << '\n';
                platform::sleep_ms(700);
        }
    }
}

} // namespace si
