// stats.cpp
#include "stats.h"

#include <fstream>

namespace si {

std::string stats_path(const std::string& user) { return user + "_stats.dat"; }

void stats_write(const std::string& user, const Stats& s) {
    std::ofstream f(stats_path(user));
    if (!f) return;
    f << s.gamesPlayed  << ' ' << s.totalScore   << ' '
      << s.aliensKilled << ' ' << s.ufosKilled   << ' '
      << s.bossesKilled << ' ' << s.deaths       << ' '
      << s.shotsFired   << ' ' << s.powerupsUsed << ' '
      << s.highestLevel << ' ' << s.highestCombo << '\n';
}

Stats stats_read(const std::string& user) {
    Stats s;
    std::ifstream f(stats_path(user));
    if (!f) return s;
    f >> s.gamesPlayed >> s.totalScore >> s.aliensKilled >> s.ufosKilled
      >> s.bossesKilled >> s.deaths >> s.shotsFired >> s.powerupsUsed
      >> s.highestLevel >> s.highestCombo;
    return s;
}

} // namespace si
