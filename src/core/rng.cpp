#include "rng.h"

namespace si {

RNG::RNG(std::uint32_t seed) : mt_(seed) {}

void RNG::reseed(std::uint32_t seed) { mt_.seed(seed); }

std::uint32_t RNG::next() { return mt_(); }

int RNG::range(int lo, int hi) {
    if (hi < lo) return lo;
    std::uniform_int_distribution<int> d(lo, hi);
    return d(mt_);
}

bool RNG::chance(int percent) { return range(0, 99) < percent; }

}
