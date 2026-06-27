#pragma once

#include "../persistence/leaderboard.h"
#include "../persistence/save_state.h"
#include "../persistence/stats.h"
#include "../persistence/achievements.h"
#include "../config/config.h"

#include <cstdint>
#include <string>
#include <vector>

namespace si {

void run_solo(int diffIdx, const std::string& user, Record& rec,
              SaveState& saved, Stats& stats,
              std::vector<Achievement>& ach);

void run_from_save(const SaveState& s, const std::string& user, Record& rec,
                   SaveState& saved, Stats& stats,
                   std::vector<Achievement>& ach);

void run_ai_demo(int diffIdx, const std::string& user,
                 Stats& stats, std::vector<Achievement>& ach,
                 const std::string& ai_profile,
                 std::uint32_t seed = 0);

void run_replay(const std::string& path, Stats& stats,
                std::vector<Achievement>& ach, const std::string& user);

void run_host(int diffIdx, const std::string& user,
              Stats& stats, std::vector<Achievement>& ach,
              int net_port);

void run_join(const std::string& user, const std::string& ip,
              Stats& stats, std::vector<Achievement>& ach,
              int net_port);

int run_train_ai(int n, int diffIdx, const std::string& ai_profile);

void show_settings(Config& cfg);

void show_credits();

void run_menu(const Config& cfg, const std::string& user, Record& rec,
              SaveState& saved, Stats& stats,
              std::vector<Achievement>& ach);

}
