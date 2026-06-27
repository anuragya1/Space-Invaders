#pragma once

#include "../core/entities.h"

#include <cstddef>
#include <vector>

namespace si {

struct InterpSnapshot {
    std::vector<Pt> aliens;

    Pt   player {};
    Pt   player2{};

    int  ufoX       = 0;
    bool ufoActive  = false;

    Pt   boss       {};
    bool bossActive = false;

    bool valid = false;
};

inline float lerp_cell(int prev, int curr, float alpha) {
    const float p = static_cast<float>(prev);
    const float c = static_cast<float>(curr);
    return p + (c - p) * alpha;
}

}
