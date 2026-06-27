#pragma once

#include "../core/constants.h"
#include <cstdint>
#include <string>

namespace si {

struct LevelFile {
    std::string   name      = "Untitled";
    std::uint32_t seed      = 1;
    std::string   author    = "unknown";
    int           moveDelay = 10;
    int           shootBase = 40;
    bool          boss      = false;
    bool          aliens[AROWS][ACOLS];
    bool          shield[2][4];
    LevelFile() {
        for (int r = 0; r < AROWS; ++r)
            for (int c = 0; c < ACOLS; ++c) aliens[r][c] = true;
        for (int r = 0; r < 2; ++r)
            for (int c = 0; c < 4; ++c) shield[r][c] = true;
    }
};

bool level_save(const std::string& path, const LevelFile& lv);
bool level_load(const std::string& path, LevelFile& lv);

}
