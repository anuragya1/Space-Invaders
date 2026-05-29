// save_state.cpp
#include "save_state.h"

#include <cstdio>
#include <fstream>

namespace si {

std::string save_path(const std::string& user) { return user + "_save.dat"; }

void save_write(const std::string& user, const SaveState& s) {
    std::ofstream f(save_path(user));
    if (!f) return;
    f << s.diffIdx << ' ' << s.score << ' ' << s.lives << ' ' << s.level << ' '
      << s.playerX << ' ' << s.alienDirX << ' ' << s.mDelay << ' ' << s.seed << '\n';
    for (int r = 0; r < AROWS; ++r)
        for (int c = 0; c < ACOLS; ++c)
            f << (int)s.aAlive[r][c] << ' ';
    f << '\n';
}

SaveState save_read(const std::string& user) {
    SaveState s;
    std::ifstream f(save_path(user));
    if (!f) return s;
    if (!(f >> s.diffIdx >> s.score >> s.lives >> s.level
            >> s.playerX >> s.alienDirX >> s.mDelay >> s.seed)) return s;
    for (int r = 0; r < AROWS; ++r)
        for (int c = 0; c < ACOLS; ++c) {
            int v; if (!(f >> v)) return s;
            s.aAlive[r][c] = (v != 0);
        }
    s.valid = true;
    return s;
}

void save_delete(const std::string& user) {
    std::remove(save_path(user).c_str());
}

} // namespace si
