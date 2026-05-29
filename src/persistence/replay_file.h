// replay_file.h - .rpl plain-text replay format.
//
//   HEADER seed=<u> diff=<i> mode=<solo|ai|coop|replay> player=<name>
//   <tick> <p1_mask> <p2_mask>
//   ...
//
// The header is whitespace-separated key=value tokens; the body is one
// frame per line. This same encoding doubles as the on-wire payload of
// the network protocol (one InputFrame per tick).
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
    // Optional integrity fields. -1 means "not embedded" (older replay).
    int                     expectedScore = -1;
    int                     expectedLevel = -1;
    std::vector<InputFrame> frames;
};

bool replay_save(const std::string& path, const Replay& rp);
bool replay_load(const std::string& path, Replay& rp);

} // namespace si
