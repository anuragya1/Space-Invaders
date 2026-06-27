#include "achievements.h"

#include <fstream>
#include <map>

namespace si {

std::vector<Achievement> achievements_default() {
    return {
        {"FIRST_BLOOD",     "First Blood          : Destroy your first alien.",            false},
        {"COMBO_5",         "Streak Master        : Land a combo x5.",                     false},
        {"COMBO_10",        "Untouchable          : Land a combo x10.",                    false},
        {"UFO_HUNTER",      "UFO Hunter           : Shoot down 5 UFOs.",                   false},
        {"BOSS_SLAYER",     "Boss Slayer          : Defeat your first boss.",              false},
        {"PERFECT_WAVE",    "Pacifist Disproven   : Clear a wave without losing a life.",  false},
        {"NIGHTMARE_WIN",   "Nightmare Survivor   : Reach level 5 on Nightmare.",          false},
        {"ULTRA_NIGHTMARE", "Damned & Determined  : Reach level 3 on Ultra-Nightmare.",    false},
        {"VETERAN_10K",     "Veteran              : Total lifetime score >= 10000.",       false},
        {"CO_OP_VICTORY",   "Brothers in Arms     : Win a co-op level.",                   false},
    };
}

std::string achievements_path(const std::string& user) { return user + "_ach.dat"; }

void achievements_write(const std::string& user, const std::vector<Achievement>& a) {
    std::ofstream f(achievements_path(user));
    if (!f) return;
    for (const auto& x : a) f << x.key << ' ' << (int)x.unlocked << '\n';
}

std::vector<Achievement> achievements_read(const std::string& user) {
    auto a = achievements_default();
    std::ifstream f(achievements_path(user));
    if (!f) return a;
    std::string key;
    int v;
    std::map<std::string, bool> m;
    while (f >> key >> v) m[key] = (v != 0);
    for (auto& x : a)
        if (auto it = m.find(x.key); it != m.end()) x.unlocked = it->second;
    return a;
}

}
