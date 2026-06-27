#pragma once

#include <string>
#include <vector>

namespace si {

struct Achievement {
    std::string key;
    std::string desc;
    bool        unlocked = false;
};

std::vector<Achievement> achievements_default();

std::string              achievements_path(const std::string& user);
void                     achievements_write(const std::string& user,
                                            const std::vector<Achievement>& a);
std::vector<Achievement> achievements_read (const std::string& user);

}
