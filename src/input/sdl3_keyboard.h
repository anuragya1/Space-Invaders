// sdl3_keyboard.h - SDL3 keyboard input source.
//
// Movement is state-polled with SDL_GetKeyboardState(), so holding left
// or right moves the player every tick.
//
// Shoot, pause, and quit are event-latched. The SDL3 main loop forwards
// key-down events to note_key_down(), then poll() consumes those pending
// flags on the next logic tick. This avoids a real missed-tap problem:
// the game ticks at 12.5 Hz, while a quick key press can begin and end
// between two polls.
//
// Rapid-fire is the exception. When the RAPID power-up is active, holding
// Space emits SHOOT on a cooldown. Game::apply_action still enforces the
// per-difficulty bullet cap.
#pragma once

#include "input_source.h"
#include <SDL3/SDL.h>

namespace si {

class SDL3Keyboard : public IInputSource {
public:
    // Forward key-down events here so short taps survive until the next
    // fixed simulation tick.
    void note_key_down(SDL_Keycode k);

    std::uint8_t poll(std::uint32_t tick, const Game& g,
                       int playerId) override;

private:
    bool pendingShoot_   = false;
    bool pendingPause_   = false;
    bool pendingQuit_    = false;
    int  autofireCooldown_ = 0;   // ticks remaining before next auto-shot

    // ~4 shots/sec at FRAME_MS = 80 ms (every 3rd tick).
    static constexpr int AUTOFIRE_COOLDOWN_TICKS = 3;
};

} // namespace si
