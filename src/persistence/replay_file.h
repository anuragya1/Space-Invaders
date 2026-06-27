#pragma once

#include "../core/action.h"
#include <cstdint>
#include <string>
#include <vector>

namespace si {

struct Replay {
    std::uint32_t           seed    = 1;
    int                     diffIdx = 1;
    std::string             modeStr = "solo";
    std::string             player  = "Player";

    int                     expectedScore = -1;
    int                     expectedLevel = -1;
    std::vector<InputFrame> frames;
};

bool replay_save(const std::string& path, const Replay& rp);
bool replay_load(const std::string& path, Replay& rp);

}
