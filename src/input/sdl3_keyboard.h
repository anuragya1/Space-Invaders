#pragma once

#include "input_source.h"
#include <SDL3/SDL.h>

namespace si {

class SDL3Keyboard : public IInputSource {
public:

    void note_key_down(SDL_Keycode k);

    std::uint8_t poll(std::uint32_t tick, const Game& g,
                       int playerId) override;

private:
    bool pendingShoot_   = false;
    bool pendingPause_   = false;
    bool pendingQuit_    = false;
    int  autofireCooldown_ = 0;

    static constexpr int AUTOFIRE_COOLDOWN_TICKS = 3;
};

}
