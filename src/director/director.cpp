#include "director.h"

#include "../game/game.h"
#include "../persistence/stats.h"

#include <algorithm>

namespace si {

namespace {
constexpr float TENSION_MIN  = -3.0f;
constexpr float TENSION_MAX  =  3.0f;
constexpr float DRIFT_RATE   = 0.02f;
constexpr float BEAT_TRIGGER =  0.55f;
constexpr float BEAT_SECONDS =  8.0f;
constexpr float BEAT_COOLDOWN = 10.0f;

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

int total_lives(const Game& g) {
    return g.player.lives + (g.hasP2 ? g.player2.lives : 0);
}
}

void Director::on_restart(const Game& g) {
    tension_           = 0.0f;
    lastLives_         = g.player.lives;
    lastP2Lives_       = g.hasP2 ? g.player2.lives : 0;
    lastLevel_         = g.level();
    lastCombo_         = g.combo();
    lastPower_         = static_cast<int>(g.player.power);
    lastP2Power_       = g.hasP2 ? static_cast<int>(g.player2.power) : 0;
    firstObserve_      = true;
    levelStartLives_   = total_lives(g);
    beat_              = Beat::STEADY;
    beatTimer_         = 0.0f;
    beatCooldown_      = 2.0f;
    mods_              = Modifiers{};
}

void Director::observe(const Game& g, float dtSec) {
    if (!enabled_) {
        mods_ = Modifiers{};
        beat_ = Beat::STEADY;
        beatTimer_ = 0.0f;
        return;
    }
    if (firstObserve_) {
        on_restart(g);
        firstObserve_ = false;
        return;
    }

    bool sawPlayerHit = false;
    bool sawPowerup = false;
    bool sawCombo = false;
    bool sawLevelClear = false;
    for (const auto& e : g.events()) {
        switch (e.type) {
            case GameEventType::PlayerHit:
                sawPlayerHit = true;
                if (e.value > 0) tension_ += TEN_DEATH * static_cast<float>(e.value);
                break;
            case GameEventType::PowerupCollected:
                sawPowerup = true;
                tension_ += TEN_POWERUP;
                break;
            case GameEventType::AlienKilled:
                sawCombo = true;
                if (e.combo == 5 || e.combo == 10 || e.combo == 15) {
                    tension_ += TEN_COMBO_5;
                }
                break;
            case GameEventType::LevelCleared:
                sawLevelClear = true;
                if (total_lives(g) >= levelStartLives_) {
                    tension_ += TEN_LEVEL_CLEAN;
                }
                levelStartLives_ = total_lives(g);
                break;
            default:
                break;
        }
    }

    if (!sawPlayerHit && g.player.lives < lastLives_) {
        const int died = lastLives_ - g.player.lives;
        tension_ += TEN_DEATH * static_cast<float>(died);
    }
    lastLives_ = g.player.lives;
    if (!sawPlayerHit && g.hasP2 && g.player2.lives < lastP2Lives_) {
        const int died = lastP2Lives_ - g.player2.lives;
        tension_ += TEN_DEATH * static_cast<float>(died);
    }
    lastP2Lives_ = g.hasP2 ? g.player2.lives : 0;

    const int curPower = static_cast<int>(g.player.power);
    if (!sawPowerup
        && lastPower_ == static_cast<int>(PUType::NONE)
        && curPower != static_cast<int>(PUType::NONE)) {
        tension_ += TEN_POWERUP;
    }
    lastPower_ = curPower;
    if (g.hasP2) {
        const int curP2Power = static_cast<int>(g.player2.power);
        if (!sawPowerup
            && lastP2Power_ == static_cast<int>(PUType::NONE)
            && curP2Power != static_cast<int>(PUType::NONE)) {
            tension_ += TEN_POWERUP;
        }
        lastP2Power_ = curP2Power;
    } else {
        lastP2Power_ = 0;
    }

    const int c = g.combo();
    if (!sawCombo && c >= 5 && lastCombo_ < 5) tension_ += TEN_COMBO_5;
    if (!sawCombo && c >= 10 && lastCombo_ < 10) tension_ += TEN_COMBO_5;
    if (!sawCombo && c >= 15 && lastCombo_ < 15) tension_ += TEN_COMBO_5;
    lastCombo_ = c;

    if (g.level() > lastLevel_) {
        if (!sawLevelClear && total_lives(g) >= levelStartLives_) {
            tension_ += TEN_LEVEL_CLEAN;
        }
        lastLevel_ = g.level();
        levelStartLives_ = total_lives(g);
    }

    if (tension_ > 0.0f) {
        tension_ -= DRIFT_RATE * dtSec;
        if (tension_ < 0.0f) tension_ = 0.0f;
    } else if (tension_ < 0.0f) {
        tension_ += DRIFT_RATE * dtSec;
        if (tension_ > 0.0f) tension_ = 0.0f;
    }

    tension_ = clampf(tension_, TENSION_MIN, TENSION_MAX);
    update_beat_(dtSec);
    recompute_mods_();
}

void Director::update_beat_(float dtSec) {
    if (beatTimer_ > 0.0f) {
        beatTimer_ -= dtSec;
        if (beatTimer_ <= 0.0f) {
            beatTimer_ = 0.0f;
            beat_ = Beat::STEADY;
            beatCooldown_ = BEAT_COOLDOWN;
        }
        return;
    }

    if (beatCooldown_ > 0.0f) {
        beatCooldown_ -= dtSec;
        if (beatCooldown_ < 0.0f) beatCooldown_ = 0.0f;
    }
    if (beatCooldown_ > 0.0f) return;

    const float p = pressure();
    if (p <= -BEAT_TRIGGER) {
        beat_ = Beat::PRESSURE_SURGE;
        beatTimer_ = BEAT_SECONDS;
    } else if (p >= BEAT_TRIGGER) {
        beat_ = Beat::RELIEF_WINDOW;
        beatTimer_ = BEAT_SECONDS;
    }
}

void Director::recompute_mods_() {

    const float p = pressure();

    if (p >= 0.0f) {

        mods_.shootMul = lerp(1.0f, 0.6f, p);
        mods_.moveMul  = lerp(1.0f, 0.7f, p);
        mods_.dropMul  = lerp(1.0f, 2.5f, p);
    } else {

        const float a = -p;
        mods_.shootMul = lerp(1.0f, 1.6f, a);
        mods_.moveMul  = lerp(1.0f, 1.5f, a);
        mods_.dropMul  = lerp(1.0f, 0.6f, a);
    }

    if (beat_ == Beat::PRESSURE_SURGE) {
        mods_.shootMul *= 1.15f;
        mods_.moveMul  *= 1.10f;
        mods_.dropMul  *= 0.80f;
    } else if (beat_ == Beat::RELIEF_WINDOW) {
        mods_.shootMul *= 0.85f;
        mods_.moveMul  *= 0.90f;
        mods_.dropMul  *= 1.35f;
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

const char* Director::beat_label() const {
    if (!enabled_) return "OFF";
    switch (beat_) {
        case Beat::PRESSURE_SURGE: return "PRESSURE SURGE";
        case Beat::RELIEF_WINDOW:  return "RELIEF WINDOW";
        case Beat::STEADY:         return "FLOW STABLE";
    }
    return "FLOW STABLE";
}

float Director::beat_progress() const {
    if (beat_ == Beat::STEADY || beatTimer_ <= 0.0f) return 0.0f;
    return clampf(beatTimer_ / BEAT_SECONDS, 0.0f, 1.0f);
}

}
