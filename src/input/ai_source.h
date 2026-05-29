// ai_source.h - heuristic AI player.
//
// Action selection: each tick, evaluate the utility of three candidate
// positions (LEFT, STAY, RIGHT) and take whichever is best.
//
//   U(x) = w_danger * danger(x)        // negative: penalize incoming bullets
//        + w_align  * alignment(x)     // positive: prefer being under a target
//        + w_pickup * pickup(x)        // positive: prefer being near power-ups
//        + w_center * center(x)        // negative: soft pull toward middle
//
// The four weights are configurable via an AIProfile struct. Three named
// presets (aggressive / defensive / balanced) are provided.
//
// Shooting is decoupled: fire when a target sits in our column and no
// friendly bullet already occupies the lane.
//
// Cost per tick: O(N_aliens + N_bullets). Trivially real-time.
#pragma once

#include "input_source.h"
#include <string>

namespace si {

struct AIProfile {
    double w_danger = 6.0;
    double w_align  = 4.0;
    double w_pickup = 2.5;
    double w_center = 0.05;
    int    cooldown = 3;        // ticks between shots
    const char* name = "balanced";
};

AIProfile ai_profile_by_name(const std::string& name);

class AISource : public IInputSource {
public:
    explicit AISource(AIProfile p = ai_profile_by_name("balanced")) : prof_(p) {}
    std::uint8_t poll(std::uint32_t tick, const Game& g, int pid) override;

    const AIProfile& profile() const { return prof_; }

private:
    AIProfile prof_;
    int       cooldown_ = 0;
};

} // namespace si
