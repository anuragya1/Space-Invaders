#pragma once

#include "../persistence/stats.h"
#include "../persistence/achievements.h"

#include <string>

namespace si::tools {

int verify_replay(const std::string& path);

int evolve_ai(int generations, int diffIdx);

int benchmark(int ticks, int diffIdx);

int ai_vs_ai(int diffIdx, const std::string& profile,
             const std::string& user, Stats& stats,
             std::vector<Achievement>& ach);

}
