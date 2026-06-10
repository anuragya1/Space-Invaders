// director.h - adaptive difficulty that watches the run.
//
// The player AI in src/input/ai_source.cpp plays the game. The Director
// does something different: it watches how the run is going and nudges
// pressure up or down. The point is not to cheat. The point is to keep
// the game from staying too flat when the player is coasting, or too
// punishing when the player is already barely hanging on.
//
// The core state is a single scalar, `tension`, which drifts toward zero
// and is nudged by player events:
//
//   +1.5 per player death        (player is struggling)
//   +0.4 per powerup pickup      (player needed help)
//   -0.5 every 5-streak combo    (player is doing well)
//   -1.0 per level cleared no-deaths
//   -0.02 per second drift toward 0
//
// `tension` clamps to [-3.0, +3.0]. We map that to pressure in [-1, +1]
// and derive three multipliers that Game applies to alien shooting,
// alien movement, and power-up drops.
//
//   pressure  =  +1  (player very stressed)  -> ease off
//                shootMul = 0.6, moveMul = 0.7, dropMul = 2.5
//
//   pressure  =   0  (steady state)
//                shootMul = 1.0, moveMul = 1.0, dropMul = 1.0
//
//   pressure  =  -1  (player coasting)        -> ramp up
//                shootMul = 1.6, moveMul = 1.5, dropMul = 0.6
//
// The values between those points are linearly interpolated.
//
// The SDL3 loop owns the wiring:
//   Director dir;
//   dir.on_restart(game);
//   // each tick after step_pub():
//   dir.observe(game, dtSec);
//   auto m = dir.modifiers();
//   game.set_director_modifiers(m.shootMul, m.moveMul, m.dropMul);
//
// The HUD reads the Director label and pressure so the player can see
// when the game is easing off or pushing harder.
#pragma once

namespace si {

class Game;

class Director {
public:
    enum class Beat {
        STEADY,
        PRESSURE_SURGE,
        RELIEF_WINDOW
    };

    struct Modifiers {
        float shootMul = 1.0f;
        float moveMul  = 1.0f;
        float dropMul  = 1.0f;
    };

    // Start a fresh run from a clean baseline.
    void on_restart(const Game& g);

    // Observe one logic tick. dtSec is wall-clock seconds, used only for
    // the slow drift back toward neutral pressure.
    void observe(const Game& g, float dtSec);

    // Current adaptive multipliers.
    const Modifiers& modifiers() const { return mods_; }

    // Current pressure, in [-1, +1].
    //   Positive  -> Director is easing off (helping the player)
    //   Negative  -> Director is ramping up (challenging the player)
    float pressure() const;

    // Human-readable status for the HUD. One of:
    //   "RAMPING UP", "PUSHING", "STEADY", "EASING OFF", "HELPING"
    const char* label() const;

    // Named pacing beat derived from sustained pressure. This makes the
    // Director visible instead of leaving it as hidden math.
    Beat beat() const { return beat_; }
    bool beat_active() const { return beat_ != Beat::STEADY; }
    const char* beat_label() const;
    float beat_seconds_left() const { return beatTimer_; }
    float beat_progress() const;

    // Runtime toggle for Settings. Disabled means neutral modifiers.
    void set_enabled(bool e) { enabled_ = e; }
    bool enabled() const     { return enabled_; }

private:
    void update_beat_(float dtSec);
    void recompute_mods_();

    // Internal state from the previous observe() call.
    float tension_   = 0.0f;
    int   lastLives_   = 0;
    int   lastP2Lives_ = 0;
    int   lastPower_ = 0;
    int   lastP2Power_ = 0;
    int   lastLevel_       = 0;
    int   lastCombo_       = 0;
    int   levelStartLives_ = 0;
    bool  enabled_ = true;
    bool  firstObserve_ = true;
    Beat  beat_ = Beat::STEADY;
    float beatTimer_ = 0.0f;
    float beatCooldown_ = 0.0f;

    Modifiers mods_{};
};

} // namespace si
