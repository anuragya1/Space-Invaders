// cli.h - command-line argument parser.
//
// Skips the menu when launched with flags - useful for demos, scripted
// multiplayer matches, replay checks, and benchmarking the AI.
//
//   si_pro                              # interactive menu
//   si_pro --help
//   si_pro --host --diff 2
//   si_pro --join 192.168.1.5
//   si_pro --replay run42.rpl
//   si_pro --ai-demo --diff 3 --seed 12345
//   si_pro --train-ai 100 --diff 2      # headless: run AI 100 times, write csv
//   si_pro --ai-profile aggressive
//   si_pro --log debug
#pragma once

#include <cstdint>
#include <string>

namespace si {

enum class CliMode {
    MENU,           // default
    HOST,
    JOIN,
    REPLAY,
    AI_DEMO,
    TRAIN_AI,       // headless: run AI N times, write CSV
    VERIFY_REPLAY,  // re-run a replay headless, check score matches header
    EVOLVE_AI,      // GA over AI weights
    BENCHMARK,      // headless timing of game loop
    SHOW_HELP,
    SHOW_VERSION,
};

struct CliArgs {
    CliMode       mode      = CliMode::MENU;
    int           diff      = -1;
    std::uint32_t seed      = 0;        // 0 = use time()
    std::string   join_ip;
    std::string   replay_path;
    int           train_n   = 0;
    int           evolve_generations = 0;
    int           bench_ticks        = 0;
    std::string   ai_profile;           // empty = use config
    std::string   log_level;            // empty = use config
    std::string   username;             // skip the callsign prompt
    bool          help_only = false;
};

// Parse argv. Returns false (and sets mode=SHOW_HELP) on parse error.
bool parse_args(int argc, char** argv, CliArgs& out);

// Print help to stdout.
void print_help(const char* prog);

} // namespace si
