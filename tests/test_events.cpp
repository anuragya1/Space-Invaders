#include "test_common.h"
#include "../src/core/action.h"
#include "../src/core/game_event.h"
#include "../src/game/game.h"
#include "../src/persistence/achievements.h"
#include "../src/persistence/stats.h"

#include <algorithm>

namespace {
bool has_event(const si::Game& g, si::GameEventType type) {
    const auto& events = g.events();
    return std::any_of(events.begin(), events.end(),
        [type](const si::GameEvent& e) { return e.type == type; });
}
}

int main() {
    using namespace si;

    {
        Stats st;
        auto ach = achievements_default();
        Game g(1, Mode::SOLO, 12345u, st, ach);
        for (auto& a : g.aliens) a.alive = false;
        g.aliens.front().alive = true;
        g.aliens.front().pos.x = g.player.pos.x;
        g.aliens.front().pos.y = g.player.pos.y - 2;

        g.step_pub(action::SHOOT, 0);
        CHECK(has_event(g, GameEventType::BulletFired));
        CHECK(has_event(g, GameEventType::AlienKilled));
    }

    {
        Stats st;
        auto ach = achievements_default();
        Game g(1, Mode::SOLO, 12345u, st, ach);
        g.powerups.emplace_back(g.player.pos.x, g.player.pos.y, PUType::RAPID);

        g.step_pub(0, 0);
        CHECK(has_event(g, GameEventType::PowerupCollected));
    }

    {
        Stats st;
        auto ach = achievements_default();
        Game g(1, Mode::SOLO, 12345u, st, ach);
        g.bullets.emplace_back(g.player.pos.x, g.player.pos.y - 1, +1, -1);

        g.step_pub(0, 0);
        CHECK(has_event(g, GameEventType::PlayerHit));
    }

    {
        Stats st;
        auto ach = achievements_default();
        Game g(1, Mode::SOLO, 12345u, st, ach);
        for (auto& a : g.aliens) a.alive = false;

        g.step_pub(0, 0);
        CHECK(has_event(g, GameEventType::LevelCleared));
    }

    {
        Stats st;
        auto ach = achievements_default();
        Game g(1, Mode::SOLO, 12345u, st, ach);
        g.boss.active = true;
        g.boss.x = g.player.pos.x;
        g.boss.y = 5;
        g.boss.maxHp = 30;
        g.boss.hp = 11;
        g.boss.stage = 1;
        g.bullets.emplace_back(g.boss.x, g.boss.y + 1, -1, 0);

        g.step_pub(0, 0);
        CHECK(has_event(g, GameEventType::BossPhaseChanged));
    }

    return test_summary("test_events");
}
