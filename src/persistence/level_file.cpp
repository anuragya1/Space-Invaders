// level_file.cpp
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
    std::string tag;
    f >> tag; std::getline(f >> std::ws, lv.name);
    f >> tag >> lv.seed >> tag >> lv.author;
    f >> tag >> lv.moveDelay >> lv.shootBase;
    int bv;
    f >> tag >> bv; lv.boss = (bv != 0);
    f >> std::ws;
    for (int r = 0; r < AROWS; ++r) {
        std::string row;
        std::getline(f, row);
        for (int c = 0; c < ACOLS && c < (int)row.size(); ++c)
            lv.aliens[r][c] = (row[c] == 'X');
    }
    for (int r = 0; r < 2; ++r) {
        std::string row;
        std::getline(f, row);
        for (int c = 0; c < 4 && c < (int)row.size(); ++c)
            lv.shield[r][c] = (row[c] == '#');
    }
    return true;
}

} // namespace si
