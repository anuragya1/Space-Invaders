// director.h - the second AI, watching the player.
//
// PURPOSE
// =======
// While the in-game AI (src/input/ai_source.cpp) plays AS the player,
// the Director plays AGAINST the player by tuning difficulty in real
// time. The intent isn't to "cheat" -- it's to keep the player in the
// flow zone, the way a movie editor paces a thriller: ease off when
// the audience is tense, ratchet up when they're getting comfortable.
//
// HOW IT WORKS
// ============
// We maintain a single scalar called `tension` that drifts toward 0
// and is nudged up/down by player events:
//
//   +1.5 per player death        (player is struggling)
//   +0.4 per powerup pickup      (player needed help)
//   -0.5 every 5-streak combo    (player is doing well)
//   -1.0 per level cleared no-deaths
//   -0.02 per second drift toward 0
//
// `tension` clamps to [-3.0, +3.0]. We map it to a 'pressure' in
// [-1, +1] = tension / 3. From pressure, we derive three multipliers
// the Game class applies to its alien-shoot rate, alien-move rate,
// and powerup-drop chance.
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
// We linearly interpolate between these three keys.
//
// USAGE
// =====
//   Director dir;
//   dir.on_restart(game);                // each new game
//   ...per tick after step_pub:
//   dir.observe(game, dtSec);
//   auto m = dir.modifiers();
//   game.set_director_modifiers(m.shootMul, m.moveMul, m.dropMul);
//
// The Director also exposes a label and a [-1,+1] pressure for the
// HUD so the player can SEE the AI working.
#pragma once

namespace si {

class Game;

class Director {
public:
    struct Modifiers {
        float shootMul = 1.0f;
        float moveMul  = 1.0f;
        float dropMul  = 1.0f;
    };

    // Reset all state to baseline; remember a snapshot of the game.
    void on_restart(const Game& g);

    // Drive one tick. dtSec is wall-time seconds since the previous
    // observe() call (not game ticks). Used for the drift-toward-0.
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

    // Allow disabling at runtime (e.g. settings toggle). When disabled
    // we return identity modifiers and pressure 0.
    void set_enabled(bool e) { enabled_ = e; }
    bool enabled() const     { return enabled_; }

private:
    void recompute_mods_();

    // ---- internal tracking ----
    float tension_   = 0.0f;
    int   lastLives_   = 0;
    int   lastP2Lives_ = 0;
    int   lastPowerUseCount_ = 0;
    int   lastLevel_       = 0;
    int   lastCombo_       = 0;
    int   levelStartDeaths_ = 0;
    bool  enabled_ = true;
    bool  firstObserve_ = true;

    Modifiers mods_{};
};

} // namespace si
