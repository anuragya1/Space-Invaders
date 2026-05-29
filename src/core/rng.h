// rng.h - deterministic seeded RNG wrapper.
//
// std::rand() is implementation-defined across libc versions; we cannot
// rely on it for replay or network sync. std::mt19937 is fixed by spec.
#pragma once

#include <cstdint>
#include <random>

namespace si {

class RNG {
public:
    explicit RNG(std::uint32_t seed = 1);

    void reseed(std::uint32_t seed);
    std::uint32_t next();
    int  range(int lo, int hi);    // inclusive
    bool chance(int percent);

private:
    std::mt19937 mt_;
};

} // namespace si
