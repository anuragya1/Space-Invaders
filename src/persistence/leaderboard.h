// leaderboard.h - top-10 records, plus personal Record.
#pragma once

#include <string>
#include <vector>

namespace si {

struct Record {
    std::string name;
    int         score = 0;
    int         level = 1;
    std::string diff;
};

inline constexpr const char* LB_FILE = "leaderboard.dat";

std::string record_path(const std::string& user);
void        record_write(const std::string& user, const Record& r);
Record      record_read (const std::string& user);

std::vector<Record> leaderboard_read();
void                leaderboard_write(std::vector<Record> lb);
void                leaderboard_submit(const Record& rec);

} // namespace si
