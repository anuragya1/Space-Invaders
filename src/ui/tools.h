// tools.h - headless analysis/benchmark commands.
//
// All of these are self-contained: they construct their own Game
// instances, run them headless, and write results to stdout / CSV.
// None of them touch the input thread or the renderer.
#pragma once

#include "../persistence/stats.h"
#include "../persistence/achievements.h"

#include <string>

namespace si::tools {

// Re-runs a replay headless and checks that the final score and level
// match the values embedded in the replay header. Returns 0 on match,
// 1 on mismatch. New replay headers carry these expected values so the
// verification is meaningful; older replays without the fields are
// accepted as "indeterminate" with exit code 0.
int verify_replay(const std::string& path);

// Genetic-algorithm AI tuning. Each individual is an AI profile
// (4 weights + cooldown). Each generation: play N games per individual,
// compute mean score, keep top half, breed (uniform crossover + mutation),
// repeat. Writes best individual + per-generation stats to ai_evolve.csv.
int evolve_ai(int generations, int diffIdx);

// Headless benchmark. Runs `ticks` ticks of the game loop with a no-op
// input source, prints wall-clock time and ticks/sec.
int benchmark(int ticks, int diffIdx);

// AI vs AI co-op mode. Two AISources with the SAME profile play
// cooperatively. Renders to terminal (so user can watch). Returns 0.
int ai_vs_ai(int diffIdx, const std::string& profile,
             const std::string& user, Stats& stats,
             std::vector<Achievement>& ach);

} // namespace si::tools
