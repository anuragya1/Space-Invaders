#pragma once

#include <cstdint>
#include <random>

namespace si {

class RNG {
public:
    explicit RNG(std::uint32_t seed = 1);

    void reseed(std::uint32_t seed);
    std::uint32_t next();
    int  range(int lo, int hi);
    bool chance(int percent);

private:
    std::mt19937 mt_;
};

}
