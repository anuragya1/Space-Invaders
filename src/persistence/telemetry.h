// telemetry.h - per-level CSV log for difficulty curve analysis.
//
// At the end of each level we append a row to <user>_curves.csv with
// the per-level stats. This is useful for checking whether difficulty
// changes feel fair instead of guessing from memory.
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

// Appends one row. Creates the file with a header if it doesn't exist.
void telemetry_append(const std::string& user, const LevelTelemetry& t);

} // namespace si
