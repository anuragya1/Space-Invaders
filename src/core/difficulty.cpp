#include "difficulty.h"

#include <algorithm>

namespace si {

static const Diff kDiffs[N_DIFFS] = {
    {"I'M TOO YOUNG TO DIE",  "Forgiving. Many lives.",         14, 55,  2, 3, 5, 1, false},
    {"HURT ME PLENTY",        "Classic balanced experience.",   10, 40,  3, 2, 3, 2, false},
    {"ULTRA-VIOLENCE",        "No mercy. No remorse.",           7, 28,  5, 2, 3, 3, false},
    {"NIGHTMARE",             "They will not stop.",             5, 18,  7, 2, 2, 4, false},
    {"ULTRA-NIGHTMARE",       "One life. No saves. Good luck.",  3, 10, 10, 2, 1, 5, true },
};

const Diff& difficulty(int idx) {
    idx = std::clamp(idx, 0, N_DIFFS - 1);
    return kDiffs[idx];
}

const Diff& difficulty_unchecked(int idx) { return kDiffs[idx]; }

}
