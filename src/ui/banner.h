#pragma once

#include "../persistence/leaderboard.h"
#include "../persistence/stats.h"
#include "../persistence/achievements.h"
#include <vector>

namespace si {

void banner();
void show_leaderboard();
void show_stats(const Stats& s);
void show_achievements(const std::vector<Achievement>& a);
void show_record(const Record& r);

void diff_menu();
int  pick_difficulty();

}
