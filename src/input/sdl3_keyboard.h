// sdl3_keyboard.h - SDL3 keyboard input source.
//
// MOVEMENT (LEFT / RIGHT)
// -----------------------
// State-polled via SDL_GetKeyboardState(). Holding the key keeps the
// player moving every tick.
//
// DISCRETE ACTIONS (SHOOT / PAUSE / QUIT)
// ---------------------------------------
// Event-latched, not state-polled. The main loop forwards every
// SDL_EVENT_KEY_DOWN to note_key_down(), which sets a pending flag.
// On the next poll() the flag is consumed and returned in the mask.
//
// This avoids the missed-tap bug: poll() runs at 12.5 Hz, but a key
// press is ~30-60 ms - faster than the poll interval - so a
// state-polling design dropped presses that started and ended between
// two poll() calls. Event-driven latching is reliable.
//
// AUTO-FIRE DURING RAPID POWER-UP
// -------------------------------
// While the player has the RAPID power-up active, holding Space
// continuously emits SHOOT (with a small per-shot cooldown of
// AUTOFIRE_COOLDOWN_TICKS, ~4 shots/sec at FRAME_MS = 80). Without
// the power-up, only taps fire. The per-difficulty playerBmax cap
// still applies via Game::apply_action.
#pragma once

#include "input_source.h"
#include <SDL3/SDL.h>

namespace si {

class SDL3Keyboard : public IInputSource {
public:
    // Forwarded from the main loop on every SDL_EVENT_KEY_DOWN.
    // Sets the relevant latch.
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
