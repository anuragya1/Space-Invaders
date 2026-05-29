// test_replay.cpp - replay file round-trip.
#include "test_common.h"
#include "../src/persistence/replay_file.h"

#include <cstdio>

int main() {
    using namespace si;

    Replay a;
    a.seed    = 12345;
    a.diffIdx = 3;
    a.modeStr = "ai";
    a.player  = "tester";
    for (std::uint32_t t = 0; t < 100; ++t)
        a.frames.push_back({ t, (std::uint8_t)(t % 5), (std::uint8_t)((t * 7) % 11) });

    const char* path = "_test_replay.rpl";
    CHECK(replay_save(path, a));

    Replay b;
    CHECK(replay_load(path, b));
    CHECK_EQ(a.seed,           b.seed);
    CHECK_EQ(a.diffIdx,        b.diffIdx);
    CHECK_EQ(a.modeStr,        b.modeStr);
    CHECK_EQ(a.player,         b.player);
    CHECK_EQ(a.frames.size(),  b.frames.size());

    for (std::size_t i = 0; i < a.frames.size(); ++i) {
        CHECK_EQ(a.frames[i].tick, b.frames[i].tick);
        CHECK_EQ(a.frames[i].p1,   b.frames[i].p1);
        CHECK_EQ(a.frames[i].p2,   b.frames[i].p2);
    }

    std::remove(path);
    return test_summary("test_replay");
}
