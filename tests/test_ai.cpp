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

    auto p_agg = ai_profile_by_name("aggressive");
    auto p_def = ai_profile_by_name("defensive");
    auto p_bal = ai_profile_by_name("balanced");
    auto p_xx  = ai_profile_by_name("unknown_xyz");
    CHECK(p_agg.w_align >  p_bal.w_align);
    CHECK(p_def.w_danger > p_bal.w_danger);
    CHECK_EQ(p_xx.name, std::string("balanced"));

    Game g(1, Mode::AI_DEMO, 12345u, st, ach);

    AISource ai(p_bal);
    auto m = ai.poll(0, g, 0);

    constexpr std::uint8_t allowed =
        action::LEFT | action::RIGHT | action::SHOOT |
        action::PAUSE | action::QUIT | action::CONSOLE;
    CHECK_EQ(m & ~allowed, 0);

    CHECK_EQ((m & action::LEFT) && (m & action::RIGHT), 0);

    for (auto& a : g.aliens) a.alive = false;
    g.aliens.front().alive   = true;
    g.aliens.front().pos.x   = g.player.pos.x;
    g.aliens.front().pos.y   = 5;
    AISource ai2(p_agg);
    auto m2 = ai2.poll(1, g, 0);
    CHECK(m2 & action::SHOOT);

    return test_summary("test_ai");
}
