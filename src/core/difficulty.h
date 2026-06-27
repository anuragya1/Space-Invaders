#pragma once

namespace si {

struct Diff {
    const char* name;
    const char* tag;
    int  moveDelay;
    int  shootBase;
    int  alienBmax;
    int  playerBmax;
    int  lives;
    int  scoreMult;
    bool oneLife;
};

inline constexpr int N_DIFFS = 5;

const Diff& difficulty(int idx);
const Diff& difficulty_unchecked(int idx);

}
