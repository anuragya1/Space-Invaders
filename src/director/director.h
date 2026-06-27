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

    void on_restart(const Game& g);

    void observe(const Game& g, float dtSec);

    const Modifiers& modifiers() const { return mods_; }

    float pressure() const;

    const char* label() const;

    Beat beat() const { return beat_; }
    bool beat_active() const { return beat_ != Beat::STEADY; }
    const char* beat_label() const;
    float beat_seconds_left() const { return beatTimer_; }
    float beat_progress() const;

    void set_enabled(bool e) { enabled_ = e; }
    bool enabled() const     { return enabled_; }

private:
    void update_beat_(float dtSec);
    void recompute_mods_();

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

}
