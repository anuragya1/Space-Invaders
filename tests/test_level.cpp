// test_level.cpp - .lvl file round-trip.
#include "test_common.h"
#include "../src/persistence/level_file.h"

#include <cstdio>

int main() {
    using namespace si;

    LevelFile a;
    a.name      = "My Test Wave";
    a.seed      = 42;
    a.author    = "anurag";
    a.moveDelay = 6;
    a.shootBase = 22;
    a.boss      = true;
    for (int r = 0; r < AROWS; ++r)
        for (int c = 0; c < ACOLS; ++c) a.aliens[r][c] = ((r + c) % 2 == 0);
    a.shield[0][0] = false; a.shield[1][3] = false;

    const char* path = "_test_level.lvl";
    CHECK(level_save(path, a));

    LevelFile b;
    CHECK(level_load(path, b));
    CHECK_EQ(a.name,      b.name);
    CHECK_EQ(a.seed,      b.seed);
    CHECK_EQ(a.author,    b.author);
    CHECK_EQ(a.moveDelay, b.moveDelay);
    CHECK_EQ(a.shootBase, b.shootBase);
    CHECK_EQ(a.boss,      b.boss);
    for (int r = 0; r < AROWS; ++r)
        for (int c = 0; c < ACOLS; ++c)
            CHECK_EQ(a.aliens[r][c], b.aliens[r][c]);
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 4; ++c)
            CHECK_EQ(a.shield[r][c], b.shield[r][c]);

    std::remove(path);
    return test_summary("test_level");
}
