/*
    Command-line parser for the terminal/developer binary.

    SDL3 is the normal player build. This binary remains useful for replay
    checks, AI runs, benchmarks, and the old terminal menu when explicitly
    requested with --legacy-menu.
*/
#pragma once

#include <cstdint>
#include <string>

namespace si {

enum class CliMode {
    MENU,
    LEGACY_MENU,
    HOST,
    JOIN,
    REPLAY,
    AI_DEMO,
    TRAIN_AI,
    VERIFY_REPLAY,
    EVOLVE_AI,
    BENCHMARK,
    SHOW_HELP,
    SHOW_VERSION,
};

struct CliArgs {
    CliMode       mode      = CliMode::MENU;
    int           diff      = -1;
    std::uint32_t seed      = 0;
    std::string   join_ip;
    std::string   replay_path;
    int           train_n   = 0;
    int           evolve_generations = 0;
    int           bench_ticks        = 0;
    std::string   ai_profile;
    std::string   log_level;
    std::string   username;
    bool          help_only = false;
};

bool parse_args(int argc, char** argv, CliArgs& out);

void print_help(const char* prog);

}
