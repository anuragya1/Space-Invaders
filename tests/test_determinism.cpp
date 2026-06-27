#include "test_common.h"
#include "../src/core/action.h"
#include "../src/core/constants.h"
#include "../src/game/game.h"
#include "../src/input/replay_source.h"
#include "../src/persistence/achievements.h"
#include "../src/persistence/replay_file.h"
#include "../src/persistence/stats.h"

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    using namespace si;

    Stats   st1, st2;
    auto    ach1 = achievements_default();
    auto    ach2 = achievements_default();

    Game g1(2, Mode::SOLO, 0xCAFEBABEu, st1, ach1);
    Game g2(2, Mode::SOLO, 0xCAFEBABEu, st2, ach2);

    SaveState s1 = g1.snap();
    SaveState s2 = g2.snap();

    CHECK_EQ(s1.diffIdx,   s2.diffIdx);
    CHECK_EQ(s1.score,     s2.score);
    CHECK_EQ(s1.lives,     s2.lives);
    CHECK_EQ(s1.level,     s2.level);
    CHECK_EQ(s1.playerX,   s2.playerX);
    CHECK_EQ(s1.alienDirX, s2.alienDirX);
    CHECK_EQ(s1.mDelay,    s2.mDelay);
    CHECK_EQ(s1.seed,      s2.seed);
    for (int r = 0; r < AROWS; ++r)
        for (int c = 0; c < ACOLS; ++c)
            CHECK_EQ(s1.aAlive[r][c], s2.aAlive[r][c]);

    Game g3(2, Mode::SOLO, 1u, st1, ach1);
    SaveState s3 = g3.snap();
    CHECK(s3.seed != s1.seed);

    Game g4(s1, st1, ach1);
    SaveState s4 = g4.snap();
    CHECK_EQ(s1.seed,      s4.seed);
    CHECK_EQ(s1.diffIdx,   s4.diffIdx);
    CHECK_EQ(s1.playerX,   s4.playerX);
    CHECK_EQ(s1.mDelay,    s4.mDelay);
    CHECK_EQ(s1.alienDirX, s4.alienDirX);

    return test_summary("test_determinism");
}
