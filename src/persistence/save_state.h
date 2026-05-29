// save_state.h - mid-game snapshot for resume.
#pragma once

#include "../core/constants.h"
#include <cstdint>
#include <string>

namespace si {

struct SaveState {
    bool          valid     = false;
    int           diffIdx   = 1;
    int           score     = 0;
    int           lives     = 3;
    int           level     = 1;
    int           playerX   = W / 2;
    int           alienDirX = 1;
    int           mDelay    = 10;
    bool          aAlive[AROWS][ACOLS] = {};
    std::uint32_t seed      = 1;
};

std::string save_path(const std::string& user);
void        save_write (const std::string& user, const SaveState& s);
SaveState   save_read  (const std::string& user);
void        save_delete(const std::string& user);

} // namespace si
