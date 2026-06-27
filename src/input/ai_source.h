#pragma once

#include "input_source.h"
#include <string>

namespace si {

struct AIProfile {
    double w_danger = 6.0;
    double w_align  = 4.0;
    double w_pickup = 2.5;
    double w_center = 0.05;
    int    cooldown = 3;
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

}
