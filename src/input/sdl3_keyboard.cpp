#include "sdl3_keyboard.h"

#include "../core/entities.h"
#include "../game/game.h"

namespace si {

void SDL3Keyboard::note_key_down(SDL_Keycode k) {

    if (k == 0x20)                   pendingShoot_ = true;
    else if (k == 0x70 || k == 0x50) pendingPause_ = true;
    else if (k == 0x71 || k == 0x51) pendingQuit_  = true;
}

std::uint8_t SDL3Keyboard::poll(std::uint32_t /*tick*/, const Game& g,
                                 int /*playerId*/) {
    std::uint8_t m = 0;

    const bool* ks = SDL_GetKeyboardState(nullptr);
    if (ks) {
        if (ks[SDL_SCANCODE_A] || ks[SDL_SCANCODE_LEFT]) {
            m |= action::LEFT;
        }
        if (ks[SDL_SCANCODE_D] || ks[SDL_SCANCODE_RIGHT]) {
            m |= action::RIGHT;
        }
    }

    if (pendingShoot_) {
        m |= action::SHOOT;
        pendingShoot_ = false;

        autofireCooldown_ = AUTOFIRE_COOLDOWN_TICKS;
    }
    if (pendingPause_) { m |= action::PAUSE; pendingPause_ = false; }
    if (pendingQuit_)  { m |= action::QUIT;  pendingQuit_  = false; }

    const bool rapidActive = (g.player.power == PUType::RAPID);
    const bool spaceHeld   = (ks && ks[SDL_SCANCODE_SPACE]);

    if (rapidActive && spaceHeld) {
        if (autofireCooldown_ <= 0 && !(m & action::SHOOT)) {

            m |= action::SHOOT;
            autofireCooldown_ = AUTOFIRE_COOLDOWN_TICKS;
        }
    }
    if (autofireCooldown_ > 0) --autofireCooldown_;

    return m;
}

}
