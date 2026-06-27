#include "ai_source.h"
#include "../game/game.h"
#include "../core/action.h"
#include "../core/constants.h"

#include <algorithm>
#include <cmath>

namespace si {

AIProfile ai_profile_by_name(const std::string& name) {
    AIProfile p;
    if (name == "aggressive") {
        p = { /*w_danger*/ 3.0, /*w_align*/ 6.0, /*w_pickup*/ 2.0,
              /*w_center*/ 0.02, /*cooldown*/ 2, "aggressive" };
    } else if (name == "defensive") {
        p = { /*w_danger*/ 10.0, /*w_align*/ 2.0, /*w_pickup*/ 3.0,
              /*w_center*/ 0.10, /*cooldown*/ 4, "defensive" };
    } else {
        p = { /*w_danger*/ 6.0, /*w_align*/ 4.0, /*w_pickup*/ 2.5,
              /*w_center*/ 0.05, /*cooldown*/ 3, "balanced" };
    }
    return p;
}

std::uint8_t AISource::poll(std::uint32_t, const Game& g, int) {
    const Player& p = g.player;
    if (p.lives <= 0) return 0;

    auto score = [&](int candX) -> double {
        double u = 0.0;

        double danger = 0.0;
        for (const auto& b : g.bullets) {
            if (!b.active || b.dir != +1) continue;
            int dx = std::abs(b.pos.x - candX);
            int dy = (p.pos.y - b.pos.y);
            if (dy <= 0) continue;
            if      (dx <= 1) danger += 100.0 / std::max(1, dy);
            else if (dx <= 2) danger +=  30.0 / std::max(1, dy);
        }
        u -= prof_.w_danger * danger;

        int bestAlign = 999;
        for (const auto& a : g.aliens)
            if (a.alive)
                bestAlign = std::min(bestAlign, std::abs(a.pos.x - candX));
        if (g.ufo.active)
            bestAlign = std::min(bestAlign, std::abs(g.ufo.x - candX));
        if (g.boss.active)
            bestAlign = std::min(bestAlign, std::abs(g.boss.x - candX));
        u += prof_.w_align * (4.0 / (1.0 + bestAlign));

        for (const auto& pu : g.powerups) {
            if (!pu.active) continue;
            int dx = std::abs(pu.pos.x - candX);
            int dy = std::abs(pu.pos.y - p.pos.y);
            if (dy < 10) u += prof_.w_pickup * 2.5 / (1.0 + dx + dy * 0.5);
        }

        u -= prof_.w_center * std::abs(candX - W / 2);

        return u;
    };

    double sL = score(p.pos.x - 1);
    double sR = score(p.pos.x + 1);
    double sS = score(p.pos.x);

    std::uint8_t mask = 0;
    if      (sL > sS + 0.1 && sL >= sR && p.pos.x > 1)    mask |= action::LEFT;
    else if (sR > sS + 0.1 && p.pos.x < W - 2)            mask |= action::RIGHT;

    if (cooldown_ > 0) --cooldown_;
    if (cooldown_ == 0) {
        bool laneClear = true;
        for (const auto& b : g.bullets) {
            if (b.active && b.dir == -1 &&
                std::abs(b.pos.x - p.pos.x) <= 1) {
                laneClear = false;
                break;
            }
        }
        if (laneClear) {
            bool target = false;
            for (const auto& a : g.aliens) {
                if (a.alive && std::abs(a.pos.x - p.pos.x) <= 1) {
                    target = true;
                    break;
                }
            }
            if (g.boss.active && std::abs(g.boss.x - p.pos.x) <= 2) target = true;
            if (g.ufo.active  && std::abs(g.ufo.x  - p.pos.x) <= 1) target = true;
            if (target) {
                mask |= action::SHOOT;
                cooldown_ = prof_.cooldown;
            }
        }
    }
    return mask;
}

}
