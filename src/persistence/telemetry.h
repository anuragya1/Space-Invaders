#pragma once

#include <string>

namespace si {

struct LevelTelemetry {
    int    level;
    int    shots_fired;
    int    aliens_killed;
    int    deaths_this_level;
    int    seconds;
    int    combo_max;
    int    score_at_end;
    int    diff_idx;
};

void telemetry_append(const std::string& user, const LevelTelemetry& t);

}
