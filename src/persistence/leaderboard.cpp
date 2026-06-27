#include "leaderboard.h"

#include <algorithm>
#include <fstream>

namespace si {

std::string record_path(const std::string& user) { return user + "_record.dat"; }

void record_write(const std::string& user, const Record& r) {
    std::ofstream f(record_path(user));
    if (!f) return;
    f << r.score << ' ' << r.level << '\n' << r.diff << '\n';
}

Record record_read(const std::string& user) {
    Record r;
    r.name = user;
    std::ifstream f(record_path(user));
    if (!f) return r;
    f >> r.score >> r.level;
    std::getline(f >> std::ws, r.diff);
    return r;
}

std::vector<Record> leaderboard_read() {
    std::vector<Record> lb;
    std::ifstream f(LB_FILE);
    if (!f) return lb;
    while (f) {
        Record r;
        std::string nm;
        if (!(f >> nm >> r.score >> r.level)) break;
        std::getline(f >> std::ws, r.diff);
        r.name = nm;
        lb.push_back(r);
    }
    return lb;
}

void leaderboard_write(std::vector<Record> lb) {
    std::sort(lb.begin(), lb.end(),
              [](const Record& a, const Record& b) { return a.score > b.score; });
    if (lb.size() > 10) lb.resize(10);
    std::ofstream f(LB_FILE);
    if (!f) return;
    for (const auto& r : lb)
        f << r.name << ' ' << r.score << ' ' << r.level << '\n' << r.diff << '\n';
}

void leaderboard_submit(const Record& rec) {
    auto lb = leaderboard_read();
    lb.erase(std::remove_if(lb.begin(), lb.end(),
             [&](const Record& r) { return r.name == rec.name; }), lb.end());
    lb.push_back(rec);
    leaderboard_write(std::move(lb));
}

}
