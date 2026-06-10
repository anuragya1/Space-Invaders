// test_director.cpp - adaptive Director pacing invariants.
#include "test_common.h"
#include "../src/director/director.h"
#include "../src/game/game.h"
#include "../src/persistence/achievements.h"
#include "../src/persistence/stats.h"

int main() {
    using namespace si;

    Stats st;
    auto ach = achievements_default();
    Game g(1, Mode::SOLO, 12345u, st, ach);
    Director director;

    director.on_restart(g);
    director.observe(g, 0.08f); // establish baseline after restart
    CHECK_EQ(director.beat(), Director::Beat::STEADY);
    CHECK_EQ(director.beat_active(), false);

    // Simulate a rough moment. Two lost lives should push pressure high
    // enough that, after the initial cooldown, the Director opens a
    // RELIEF WINDOW.
    g.player.lives -= 2;
    for (int i = 0; i < 32; ++i) {
        director.observe(g, 0.08f);
    }

    CHECK_EQ(director.beat(), Director::Beat::RELIEF_WINDOW);
    CHECK(director.beat_active());
    CHECK(director.beat_seconds_left() > 0.0f);
    CHECK(director.modifiers().shootMul < 1.0f);
    CHECK(director.modifiers().dropMul > 1.0f);

    director.set_enabled(false);
    director.observe(g, 0.08f);
    CHECK_EQ(director.pressure(), 0.0f);
    CHECK_EQ(director.beat(), Director::Beat::STEADY);
    CHECK_EQ(director.modifiers().shootMul, 1.0f);
    CHECK_EQ(director.modifiers().moveMul, 1.0f);
    CHECK_EQ(director.modifiers().dropMul, 1.0f);

    return test_summary("test_director");
}
