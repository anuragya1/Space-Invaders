#include "cli.h"

#include <cstring>
#include <iostream>
#include <string>

namespace si {

static bool eat_value(int argc, char** argv, int& i, std::string& v) {
    if (i + 1 >= argc) return false;
    v = argv[++i];
    return true;
}

bool parse_args(int argc, char** argv, CliArgs& out) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--help" || a == "-h") {
            out.mode = CliMode::SHOW_HELP;
            out.help_only = true;
            return true;
        }
        else if (a == "--version" || a == "-V") {
            out.mode = CliMode::SHOW_VERSION;
            out.help_only = true;
            return true;
        }
        else if (a == "--legacy-menu") out.mode = CliMode::LEGACY_MENU;
        else if (a == "--host")    out.mode = CliMode::HOST;
        else if (a == "--join") {
            out.mode = CliMode::JOIN;
            if (!eat_value(argc, argv, i, out.join_ip)) return false;
        }
        else if (a == "--replay") {
            out.mode = CliMode::REPLAY;
            if (!eat_value(argc, argv, i, out.replay_path)) return false;
        }
        else if (a == "--verify-replay") {
            out.mode = CliMode::VERIFY_REPLAY;
            if (!eat_value(argc, argv, i, out.replay_path)) return false;
        }
        else if (a == "--ai-demo") out.mode = CliMode::AI_DEMO;
        else if (a == "--train-ai") {
            out.mode = CliMode::TRAIN_AI;
            std::string v;
            if (!eat_value(argc, argv, i, v)) return false;
            try { out.train_n = std::stoi(v); }
            catch (...) { return false; }
        }
        else if (a == "--evolve-ai") {
            out.mode = CliMode::EVOLVE_AI;
            std::string v;
            if (!eat_value(argc, argv, i, v)) return false;
            try { out.evolve_generations = std::stoi(v); }
            catch (...) { return false; }
        }
        else if (a == "--benchmark") {
            out.mode = CliMode::BENCHMARK;
            std::string v;
            if (!eat_value(argc, argv, i, v)) return false;
            try { out.bench_ticks = std::stoi(v); }
            catch (...) { return false; }
        }
        else if (a == "--diff") {
            std::string v;
            if (!eat_value(argc, argv, i, v)) return false;
            try { out.diff = std::stoi(v) - 1; }
            catch (...) { return false; }
        }
        else if (a == "--seed") {
            std::string v;
            if (!eat_value(argc, argv, i, v)) return false;
            try { out.seed = (std::uint32_t)std::stoul(v); }
            catch (...) { return false; }
        }
        else if (a == "--ai-profile") {
            if (!eat_value(argc, argv, i, out.ai_profile)) return false;
        }
        else if (a == "--log") {
            if (!eat_value(argc, argv, i, out.log_level)) return false;
        }
        else if (a == "--user") {
            if (!eat_value(argc, argv, i, out.username)) return false;
        }
        else {
            std::cerr << "Unknown argument: " << a << "\n";
            out.mode = CliMode::SHOW_HELP;
            return false;
        }
    }
    return true;
}

void print_help(const char* prog) {
    std::cout <<
"Space Invaders - Pro Edition\n"
"\n"
"USAGE\n"
"  " << prog << " [options]\n"
"\n"
"OPTIONS\n"
"  --help, -h               Show this help and exit.\n"
"  --version, -V            Show version and exit.\n"
"  --user NAME              Use callsign NAME, skip the prompt.\n"
"  --legacy-menu            Open the old terminal menu.\n"
"  --host                   Launch directly as co-op host.\n"
"  --join IP                Launch directly as co-op client.\n"
"  --replay FILE.rpl        Play back a recorded session.\n"
"  --verify-replay FILE     Re-run replay headless, check score matches header.\n"
"  --ai-demo                Run the AI bot.\n"
"  --train-ai N             Headless: run AI N times, write ai_train.csv.\n"
"  --evolve-ai N            GA over AI weights for N generations.\n"
"  --benchmark N            Headless timing: run N ticks, print results.\n"
"  --diff 1..5              Difficulty preset.\n"
"  --seed N                 RNG seed (0 = wall-clock).\n"
"  --ai-profile P           aggressive | defensive | balanced.\n"
"  --log LEVEL              debug | info | warn | error | off.\n"
"\n"
"EXAMPLES\n"
"  " << prog << "                              # SDL3-first notice\n"
"  " << prog << " --legacy-menu                # old terminal menu\n"
"  " << prog << " --host --diff 2              # host a co-op game\n"
"  " << prog << " --join 127.0.0.1             # join one\n"
"  " << prog << " --ai-demo --seed 42          # deterministic AI run\n"
"  " << prog << " --train-ai 100 --diff 3      # benchmark AI performance\n"
"  " << prog << " --evolve-ai 20               # evolve AI weights\n"
"  " << prog << " --verify-replay run.rpl      # check replay integrity\n";
}

}
