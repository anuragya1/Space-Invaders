// director.cpp - the second AI: tunes difficulty to keep the player in flow.
#include "director.h"

#include "../game/game.h"
#include "../persistence/stats.h"

#include <algorithm>

namespace si {

namespace {
constexpr float TENSION_MIN  = -3.0f;
constexpr float TENSION_MAX  =  3.0f;
constexpr float DRIFT_RATE   = 0.02f;     // per second toward zero

constexpr float TEN_DEATH        = 1.5f;
constexpr float TEN_POWERUP      = 0.4f;
constexpr float TEN_COMBO_5      = -0.5f;
constexpr float TEN_LEVEL_CLEAN  = -1.0f;

float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}
} // namespace

void Director::on_restart(const Game& g) {
    tension_           = 0.0f;
    lastLives_         = g.player.lives;
    lastP2Lives_       = g.hasP2 ? g.player2.lives : 0;
    lastLevel_         = g.level();
    lastCombo_         = g.combo();
    lastPowerUseCount_ = 0;
    firstObserve_      = true;
    levelStartDeaths_  = 0;
    mods_              = Modifiers{};
}

void Director::observe(const Game& g, float dtSec) {
    if (!enabled_) {
        mods_ = Modifiers{};
        return;
    }
    if (firstObserve_) {
        on_restart(g);
        firstObserve_ = false;
        return;
    }

    // ---- Event-based tension changes ----

    // Deaths since last tick (counts P1 + P2 if active).
    if (g.player.lives < lastLives_) {
        const int died = lastLives_ - g.player.lives;
        tension_ += TEN_DEATH * static_cast<float>(died);
    }
    lastLives_ = g.player.lives;
    if (g.hasP2 && g.player2.lives < lastP2Lives_) {
        const int died = lastP2Lives_ - g.player2.lives;
        tension_ += TEN_DEATH * static_cast<float>(died);
    }
    lastP2Lives_ = g.hasP2 ? g.player2.lives : 0;

    // Powerup pickups since last tick. We can't see the events directly,
    // so we infer from a transition NONE -> something. Same heuristic as
    // the audio observer.
    // (We approximate: any time player.power changes from NONE to set.)
    // Tracked by lastPowerUseCount_ = how many times we saw a transition.
    // Direct check: if the current power state is non-NONE and the last
    // observed level/score differs, we may double-count - acceptable
    // given the small magnitude.
    // For simplicity here we use combo deltas as a proxy for "doing well"
    // and skip a dedicated powerup hook.

    // Combo high-water-mark crossings of 5, 10, 15 ...
    const int c = g.combo();
    if (c >= 5 && lastCombo_ < 5) tension_ += TEN_COMBO_5;
    if (c >= 10 && lastCombo_ < 10) tension_ += TEN_COMBO_5;
    if (c >= 15 && lastCombo_ < 15) tension_ += TEN_COMBO_5;
    lastCombo_ = c;

    // Level cleared without dying since this level started.
    if (g.level() > lastLevel_) {
        if (g.player.lives >= lastLives_) {
            // Cleared the level without losing a life (level changed
            // and lives didn't drop on this tick - approximation).
            tension_ += TEN_LEVEL_CLEAN;
        }
        lastLevel_ = g.level();
    }

    // ---- Continuous drift toward zero ----
    if (tension_ > 0.0f) {
        tension_ -= DRIFT_RATE * dtSec;
        if (tension_ < 0.0f) tension_ = 0.0f;
    } else if (tension_ < 0.0f) {
        tension_ += DRIFT_RATE * dtSec;
        if (tension_ > 0.0f) tension_ = 0.0f;
    }

    tension_ = clampf(tension_, TENSION_MIN, TENSION_MAX);
    recompute_mods_();
}

void Director::recompute_mods_() {
    // pressure in [-1, +1]
    const float p = pressure();

    // Three keys: pressure = +1, 0, -1. Linearly interpolate.
    if (p >= 0.0f) {
        // 0 .. +1: ease off as p rises
        mods_.shootMul = lerp(1.0f, 0.6f, p);
        mods_.moveMul  = lerp(1.0f, 0.7f, p);
        mods_.dropMul  = lerp(1.0f, 2.5f, p);
    } else {
        // 0 .. -1: ramp up as p falls
        const float a = -p;
        mods_.shootMul = lerp(1.0f, 1.6f, a);
        mods_.moveMul  = lerp(1.0f, 1.5f, a);
        mods_.dropMul  = lerp(1.0f, 0.6f, a);
    }
}

float Director::pressure() const {
    if (!enabled_) return 0.0f;
    return clampf(tension_ / TENSION_MAX, -1.0f, 1.0f);
}

const char* Director::label() const {
    if (!enabled_) return "OFF";
    const float p = pressure();
    if (p >  0.7f) return "HELPING";
    if (p >  0.3f) return "EASING OFF";
    if (p > -0.3f) return "STEADY";
    if (p > -0.7f) return "PUSHING";
    return "RAMPING UP";
}

} // namespace si
