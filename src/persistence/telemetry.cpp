#include "telemetry.h"

#include <fstream>

namespace si {

static std::string telemetry_path(const std::string& user) {
    return user + "_curves.csv";
}

void telemetry_append(const std::string& user, const LevelTelemetry& t) {
    std::string path = telemetry_path(user);
    bool need_header;
    {
        std::ifstream check(path);
        need_header = !check.good();
    }
    std::ofstream f(path, std::ios::app);
    if (!f) return;
    if (need_header)
        f << "level,diff,shots,kills,deaths,seconds,combo,score\n";
    f << t.level << ',' << t.diff_idx << ',' << t.shots_fired << ','
      << t.aliens_killed << ',' << t.deaths_this_level << ','
      << t.seconds << ',' << t.combo_max << ',' << t.score_at_end << '\n';
}

}
