// test_ai.cpp - AI source basic invariants.
//
// We can't run a full game headless (the Game's run() renders) but we
// CAN construct an AISource against a hand-built Game state and check
// that the AI behaves sensibly:
//   - returns a valid mask
//   - doesn't try to move out of bounds
//   - fires when aligned with an alien
#include "test_common.h"
#include "../src/core/action.h"
#include "../src/game/game.h"
#include "../src/input/ai_source.h"
#include "../src/persistence/achievements.h"
#include "../src/persistence/stats.h"

int main() {
    using namespace si;

    Stats st;
    auto ach = achievements_default();

    // Profile resolution.
    auto p_agg = ai_profile_by_name("aggressive");
    auto p_def = ai_profile_by_name("defensive");
    auto p_bal = ai_profile_by_name("balanced");
    auto p_xx  = ai_profile_by_name("unknown_xyz");  // -> balanced
    CHECK(p_agg.w_align >  p_bal.w_align);
    CHECK(p_def.w_danger > p_bal.w_danger);
    CHECK_EQ(p_xx.name, std::string("balanced"));

    // Build a Game; the AI should choose a valid mask.
    Game g(1, Mode::AI_DEMO, 12345u, st, ach);

    AISource ai(p_bal);
    auto m = ai.poll(0, g, 0);

    // Result must use only the action bits we defined.
    constexpr std::uint8_t allowed =
        action::LEFT | action::RIGHT | action::SHOOT |
        action::PAUSE | action::QUIT | action::CONSOLE;
    CHECK_EQ(m & ~allowed, 0);

    // Should not request both LEFT and RIGHT in the same tick.
    CHECK_EQ((m & action::LEFT) && (m & action::RIGHT), 0);

    // Force an alien directly above the player -> AI should want to shoot.
    // (We tweak a public state member through the Game's vectors.)
    for (auto& a : g.aliens) a.alive = false;
    g.aliens.front().alive   = true;
    g.aliens.front().pos.x   = g.player.pos.x;
    g.aliens.front().pos.y   = 5;
    AISource ai2(p_agg);
    auto m2 = ai2.poll(1, g, 0);
    CHECK(m2 & action::SHOOT);

    return test_summary("test_ai");
}
