// tools.cpp
#include "tools.h"

#include "../core/action.h"
#include "../core/colors.h"
#include "../core/difficulty.h"
#include "../debug/logger.h"
#include "../game/game.h"
#include "../input/ai_source.h"
#include "../input/input_source.h"
#include "../input/keyboard_source.h"
#include "../input/replay_source.h"
#include "../persistence/replay_file.h"
#include "../platform/platform.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

namespace si::tools {

// Replay verifier.
int verify_replay(const std::string& path) {
    Replay rp;
    if (!replay_load(path, rp)) {
        std::cerr << "Cannot read replay: " << path << '\n';
        return 2;
    }
    std::cout << "Verifying " << path << " ...\n";
    std::cout << "  seed=" << rp.seed
              << "  diff=" << difficulty_unchecked(rp.diffIdx).name
              << "  player=" << rp.player
              << "  frames=" << rp.frames.size() << '\n';

    Stats st; auto ach = achievements_default();
    ReplaySource r1(rp.frames, 1), r2(rp.frames, 2);
    Game g(rp.diffIdx, Mode::REPLAY, rp.seed, st, ach);
    // Run long enough to cover all frames.
    std::uint32_t budget = rp.frames.empty() ? 10000u
                          : (rp.frames.back().tick + 200u);
    g.run_headless(&r1, &r2, budget);

    int actual_score = g.score();
    int actual_level = g.level();
    std::cout << "  Replayed score=" << actual_score
              << " level=" << actual_level << '\n';

    if (rp.expectedScore < 0 && rp.expectedLevel < 0) {
        std::cout << color::BYELLOW
                  << "  (no embedded expectations -- INDETERMINATE OK)\n"
                  << color::RST;
        return 0;
    }
    bool ok = (rp.expectedScore < 0 || rp.expectedScore == actual_score)
           && (rp.expectedLevel < 0 || rp.expectedLevel == actual_level);
    if (ok) {
        std::cout << color::BGREEN
                  << "  OK  (matches embedded expectations)\n" << color::RST;
        return 0;
    }
    std::cout << color::BRED
              << "  FAIL  expected score=" << rp.expectedScore
              << " level=" << rp.expectedLevel
              << ", got score=" << actual_score
              << " level=" << actual_level << '\n' << color::RST;
    return 1;
}

// Headless benchmark.
int benchmark(int ticks, int diffIdx) {
    Stats st; auto ach = achievements_default();
    AISource ai(ai_profile_by_name("aggressive"));
    Game g(diffIdx, Mode::AI_DEMO, (std::uint32_t)std::time(nullptr), st, ach);

    auto t0 = std::chrono::steady_clock::now();
    g.run_headless(&ai, nullptr, (std::uint32_t)ticks);
    auto t1 = std::chrono::steady_clock::now();
    double secs = std::chrono::duration<double>(t1 - t0).count();
    double rate = ticks / std::max(secs, 1e-9);

    std::cout << "  Ticks         : " << ticks << '\n'
              << "  Wall-clock    : " << std::fixed << std::setprecision(4)
              << secs << " s\n"
              << "  Ticks/sec     : " << std::fixed << std::setprecision(0)
              << rate << '\n'
              << "  us/tick (avg) : " << std::fixed << std::setprecision(2)
              << (1e6 * secs / std::max(1, ticks)) << '\n';
    std::cout << "  Final score   : " << g.score()
              << "  level         : " << g.level() << '\n';
    return 0;
}

// Genetic search over AI weights.
//
// Genome: 5 floats {w_danger, w_align, w_pickup, w_center, cooldown}.
// Fitness: mean score across K games on the given difficulty.
//
// Population: 12 individuals.
// Per gen: evaluate all, keep top 4 (elitism), produce 8 offspring via
// crossover + Gaussian mutation. Write each generation's best to CSV.

namespace {
struct Genome { double d, a, p, c; int cd; double fitness = 0.0; };

double clampd(double v, double lo, double hi) { return std::max(lo, std::min(hi, v)); }
int    clampi(int v, int lo, int hi)          { return std::max(lo, std::min(hi, v)); }

double evaluate(const Genome& g, int diffIdx, int games_per) {
    Stats st; auto ach = achievements_default();
    double total = 0;
    for (int i = 0; i < games_per; ++i) {
        AIProfile prof;
        prof.w_danger = g.d; prof.w_align = g.a;
        prof.w_pickup = g.p; prof.w_center = g.c;
        prof.cooldown = g.cd; prof.name = "evolved";
        AISource ai(prof);
        std::uint32_t seed = (std::uint32_t)(0xC0FFEEu + i * 1009u + (uint32_t)(g.d * 13.0));
        Game game(diffIdx, Mode::AI_DEMO, seed, st, ach);
        game.run_headless(&ai, nullptr, 4000);
        total += game.score();
    }
    return total / games_per;
}
} // namespace

int evolve_ai(int generations, int diffIdx) {
    const int POP = 12, ELITE = 4, GAMES_PER = 4;
    std::mt19937 rng((std::uint32_t)std::time(nullptr));
    std::uniform_real_distribution<double> uw(0.5, 12.0);
    std::uniform_real_distribution<double> uc(0.0, 0.20);
    std::uniform_int_distribution<int>     ucd(1, 6);
    std::normal_distribution<double>       n01(0, 1);

    std::vector<Genome> pop(POP);
    for (auto& g : pop) {
        g.d = uw(rng); g.a = uw(rng);
        g.p = uw(rng); g.c = uc(rng);
        g.cd = ucd(rng);
    }

    std::ofstream f("ai_evolve.csv");
    f << "gen,best,mean,d,a,p,c,cd\n";

    std::cout << color::BCYAN << "  Evolving AI: " << generations
              << " generations, pop=" << POP
              << ", games/individual=" << GAMES_PER
              << ", diff=" << difficulty_unchecked(diffIdx).name
              << color::RST << "\n\n";

    Genome best_ever{};
    for (int gen = 1; gen <= generations; ++gen) {
        // Evaluate.
        double sum = 0;
        for (auto& g : pop) {
            g.fitness = evaluate(g, diffIdx, GAMES_PER);
            sum += g.fitness;
        }
        std::sort(pop.begin(), pop.end(),
                  [](const Genome& a, const Genome& b) {
                      return a.fitness > b.fitness;
                  });
        if (pop[0].fitness > best_ever.fitness) best_ever = pop[0];

        f << gen << ',' << pop[0].fitness << ',' << (sum / POP) << ','
          << pop[0].d << ',' << pop[0].a << ',' << pop[0].p << ','
          << pop[0].c << ',' << pop[0].cd << '\n';
        f.flush();

        std::cout << "  gen " << std::setw(3) << gen
                  << " best=" << std::fixed << std::setprecision(0) << pop[0].fitness
                  << " mean=" << std::fixed << std::setprecision(0) << (sum / POP)
                  << "  [d=" << std::fixed << std::setprecision(2) << pop[0].d
                  << " a=" << pop[0].a << " p=" << pop[0].p
                  << " c=" << pop[0].c << " cd=" << pop[0].cd << "]\n";

        // Breed: pop[0..ELITE-1] survives as-is. Rest replaced via
        // uniform crossover from random pair of elites + mutation.
        std::uniform_int_distribution<int> uparent(0, ELITE - 1);
        for (int i = ELITE; i < POP; ++i) {
            const auto& pA = pop[uparent(rng)];
            const auto& pB = pop[uparent(rng)];
            Genome child;
            child.d  = (rng() & 1) ? pA.d  : pB.d;
            child.a  = (rng() & 1) ? pA.a  : pB.a;
            child.p  = (rng() & 1) ? pA.p  : pB.p;
            child.c  = (rng() & 1) ? pA.c  : pB.c;
            child.cd = (rng() & 1) ? pA.cd : pB.cd;
            // Gaussian mutation.
            child.d  = clampd(child.d  + n01(rng) * 0.8,  0.5, 15.0);
            child.a  = clampd(child.a  + n01(rng) * 0.8,  0.5, 15.0);
            child.p  = clampd(child.p  + n01(rng) * 0.5,  0.5, 10.0);
            child.c  = clampd(child.c  + n01(rng) * 0.03, 0.0,  0.3);
            child.cd = clampi(child.cd + (int)std::round(n01(rng) * 1.0), 1, 8);
            pop[i] = child;
        }
    }

    std::cout << color::BGREEN
              << "\n  Best ever: fitness=" << std::fixed << std::setprecision(0)
              << best_ever.fitness
              << "  [d=" << std::fixed << std::setprecision(3) << best_ever.d
              << " a=" << best_ever.a << " p=" << best_ever.p
              << " c=" << best_ever.c << " cd=" << best_ever.cd << "]\n"
              << "  Wrote ai_evolve.csv\n" << color::RST;
    return 0;
}

// AI vs AI co-op.
//
// Two AISources play together. We use a co-op mode so Game creates both
// player slots, then dispatch each playerId to its own AI source.
int ai_vs_ai(int diffIdx, const std::string& profile,
             const std::string& user, Stats& stats,
             std::vector<Achievement>& ach) {
    (void)user;
    std::uint32_t seed = (std::uint32_t)std::time(nullptr);
    LOG_INFO("ai_vs_ai: profile=" << profile << " seed=" << seed);

    AISource ai_p1(ai_profile_by_name(profile));
    AISource ai_p2(ai_profile_by_name(profile));

    // We need two independent AI sources, one per slot. Wrap them in
    // a simple multiplexer that dispatches on playerId.
    struct Dual : IInputSource {
        AISource& a; AISource& b;
        Dual(AISource& x, AISource& y) : a(x), b(y) {}
        std::uint8_t poll(std::uint32_t t, const Game& g, int pid) override {
            return pid == 0 ? a.poll(t, g, 0) : b.poll(t, g, 1);
        }
    } dual(ai_p1, ai_p2);

    Game game(diffIdx, Mode::COOP_HOST, seed, stats, ach);
    game.run(&dual, &dual);
    std::cout << "\n  Press ENTER to continue..."; std::cin.get();
    return 0;
}

} // namespace si::tools
