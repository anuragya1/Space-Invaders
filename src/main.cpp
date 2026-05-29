// main.cpp - entry point.
//
// Loads si_pro.cfg (if present), parses CLI args, configures the logger,
// then either dispatches a one-shot CLI mode or enters the interactive
// menu loop.
#include "config/cli.h"
#include "config/config.h"
#include "core/version.h"
#include "debug/logger.h"
#include "i18n/strings.h"
#include "persistence/leaderboard.h"
#include "persistence/save_state.h"
#include "persistence/stats.h"
#include "persistence/achievements.h"
#include "platform/platform.h"
#include "ui/banner.h"
#include "ui/menus.h"
#include "ui/tools.h"
#include "ui/ui_options.h"

#include <cctype>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

namespace {

si::LogLevel parse_log_level(const std::string& s) {
    if (s == "debug") return si::LogLevel::DBG;
    if (s == "info")  return si::LogLevel::INFO;
    if (s == "warn")  return si::LogLevel::WARN;
    if (s == "error") return si::LogLevel::ERR;
    return si::LogLevel::OFF;
}

std::string sanitize_username(const std::string& s) {
    std::string out;
    for (char c : s) if (std::isalnum((unsigned char)c) || c == '_') out += c;
    if (out.empty()) out = "Player";
    return out;
}

} // namespace

int main(int argc, char** argv) {
    using namespace si;

    std::srand((unsigned)std::time(nullptr));
    platform::enable_ansi();
    platform::net_init();

    // ---- load config (optional) ----
    Config cfg;
    load_config("si_pro.cfg", cfg);

    // ---- parse CLI flags ----
    CliArgs args;
    if (!parse_args(argc, argv, args)) {
        print_help(argv[0]);
        platform::net_cleanup();
        return 2;
    }
    if (args.mode == CliMode::SHOW_VERSION) {
        std::cout << "si_pro " << si::version()
                  << " (built " << si::build_date() << ")\n";
        platform::net_cleanup();
        return 0;
    }
    if (args.mode == CliMode::SHOW_HELP) {
        print_help(argv[0]);
        platform::net_cleanup();
        return 0;
    }

    // CLI overrides config.
    if (!args.ai_profile.empty()) cfg.ai_profile = args.ai_profile;
    if (!args.log_level.empty())  cfg.log_level  = args.log_level;
    if (args.seed != 0)           cfg.ai_seed    = args.seed;

    // ---- propagate UI options to the global the renderer reads ----
    ui::opts().colorblind = cfg.colorblind;
    ui::opts().sound      = cfg.sound;
    i18n::set_language(cfg.language);

    // ---- configure logger ----
    // Append PID to the log filename so co-located processes (host + client
    // on the same machine for loopback testing) don't clobber each other.
    std::string log_path = cfg.log_file;
    {
        auto dot = log_path.find_last_of('.');
#ifdef _WIN32
        long pid = (long)GetCurrentProcessId();
#else
        long pid = (long)getpid();
#endif
        std::string suffix = "." + std::to_string(pid);
        if (dot != std::string::npos) log_path.insert(dot, suffix);
        else                            log_path += suffix;
    }
    Logger::get().configure(parse_log_level(cfg.log_level), log_path);
    LOG_INFO("si_pro start");

    // ---- get user (CLI flag, or prompt) ----
    // Headless modes don't need a callsign; only the interactive menu
    // and the player-driven modes (host/join/replay viewing/ai-demo) do.
    bool needs_user = (args.mode == CliMode::MENU
                    || args.mode == CliMode::HOST
                    || args.mode == CliMode::JOIN
                    || args.mode == CliMode::REPLAY
                    || args.mode == CliMode::AI_DEMO);
    std::string user = args.username;
    if (user.empty() && needs_user) {
        banner();
        std::cout << "  Enter your callsign: ";
        std::getline(std::cin, user);
    }
    if (user.empty()) user = "anon";
    user = sanitize_username(user);
    LOG_INFO("user=" << user);

    // ---- load per-user state ----
    Record    rec   = record_read(user);
    SaveState saved = save_read  (user);
    Stats     stats = stats_read (user);
    auto      ach   = achievements_read(user);

    int rc = 0;
    switch (args.mode) {
        case CliMode::HOST: {
            int di = args.diff >= 0 ? args.diff : cfg.default_diff;
            run_host(di, user, stats, ach, cfg.net_port);
            break;
        }
        case CliMode::JOIN:
            run_join(user, args.join_ip, stats, ach, cfg.net_port);
            break;

        case CliMode::REPLAY:
            run_replay(args.replay_path, stats, ach, user);
            break;

        case CliMode::AI_DEMO: {
            int di = args.diff >= 0 ? args.diff : cfg.default_diff;
            run_ai_demo(di, user, stats, ach, cfg.ai_profile, args.seed);
            break;
        }
        case CliMode::TRAIN_AI: {
            int di = args.diff >= 0 ? args.diff : cfg.default_diff;
            rc = run_train_ai(args.train_n, di, cfg.ai_profile);
            break;
        }
        case CliMode::VERIFY_REPLAY:
            rc = tools::verify_replay(args.replay_path);
            break;

        case CliMode::EVOLVE_AI: {
            int di = args.diff >= 0 ? args.diff : cfg.default_diff;
            rc = tools::evolve_ai(args.evolve_generations, di);
            break;
        }
        case CliMode::BENCHMARK: {
            int di = args.diff >= 0 ? args.diff : cfg.default_diff;
            rc = tools::benchmark(args.bench_ticks, di);
            break;
        }
        case CliMode::SHOW_HELP:
        case CliMode::SHOW_VERSION:
            // handled above
            break;

        case CliMode::MENU:
        default:
            run_menu(cfg, user, rec, saved, stats, ach);
            break;
    }

    LOG_INFO("si_pro exit rc=" << rc);
    platform::net_cleanup();
    return rc;
}
