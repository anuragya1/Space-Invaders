// stats.h - lifetime statistics.
#pragma once

#include <string>

namespace si {

struct Stats {
    int gamesPlayed   = 0;
    int totalScore    = 0;
    int aliensKilled  = 0;
    int ufosKilled    = 0;
    int bossesKilled  = 0;
    int deaths        = 0;
    int shotsFired    = 0;
    int powerupsUsed  = 0;
    int highestLevel  = 0;
    int highestCombo  = 0;
};

std::string stats_path(const std::string& user);
void        stats_write(const std::string& user, const Stats& s);
Stats       stats_read (const std::string& user);

} // namespace si
