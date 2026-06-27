#include "level_file.h"

#include <fstream>

namespace si {

bool level_save(const std::string& path, const LevelFile& lv) {
    std::ofstream f(path);
    if (!f) return false;
    f << "NAME "   << lv.name << '\n';
    f << "SEED "   << lv.seed << " AUTHOR " << lv.author << '\n';
    f << "TIMING " << lv.moveDelay << ' ' << lv.shootBase << '\n';
    f << "BOSS "   << (lv.boss ? 1 : 0) << '\n';
    for (int r = 0; r < AROWS; ++r) {
        for (int c = 0; c < ACOLS; ++c) f << (lv.aliens[r][c] ? 'X' : '.');
        f << '\n';
    }
    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 4; ++c) f << (lv.shield[r][c] ? '#' : '.');
        f << '\n';
    }
    return true;
}

bool level_load(const std::string& path, LevelFile& lv) {
    std::ifstream f(path);
    if (!f) return false;

    LevelFile parsed;
    std::string tag;
    if (!(f >> tag) || tag != "NAME") return false;
    std::getline(f >> std::ws, parsed.name);
    if (parsed.name.empty()) return false;

    if (!(f >> tag) || tag != "SEED") return false;
    if (!(f >> parsed.seed)) return false;
    if (!(f >> tag) || tag != "AUTHOR") return false;
    if (!(f >> parsed.author)) return false;

    if (!(f >> tag) || tag != "TIMING") return false;
    if (!(f >> parsed.moveDelay >> parsed.shootBase)) return false;
    if (parsed.moveDelay <= 0 || parsed.shootBase <= 0) return false;

    int bv;
    if (!(f >> tag) || tag != "BOSS") return false;
    if (!(f >> bv)) return false;
    parsed.boss = (bv != 0);

    f >> std::ws;
    for (int r = 0; r < AROWS; ++r) {
        std::string row;
        if (!std::getline(f, row) || (int)row.size() < ACOLS) return false;
        for (int c = 0; c < ACOLS; ++c) {
            if (row[c] != 'X' && row[c] != '.') return false;
            parsed.aliens[r][c] = (row[c] == 'X');
        }
    }
    for (int r = 0; r < 2; ++r) {
        std::string row;
        if (!std::getline(f, row) || (int)row.size() < 4) return false;
        for (int c = 0; c < 4; ++c) {
            if (row[c] != '#' && row[c] != '.') return false;
            parsed.shield[r][c] = (row[c] == '#');
        }
    }

    lv = parsed;
    return true;
}

}
