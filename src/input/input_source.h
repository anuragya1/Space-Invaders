#pragma once

#include "../core/action.h"
#include <atomic>
#include <cstdint>

namespace si {

class Game;

struct IInputSource {
    virtual ~IInputSource() = default;
    virtual std::uint8_t poll(std::uint32_t tick,
                              const Game& g,
                              int playerId) = 0;
};

struct InputState {
    std::atomic<bool> left   {false};
    std::atomic<bool> right  {false};
    std::atomic<bool> shoot  {false};
    std::atomic<bool> quit   {false};
    std::atomic<bool> pause  {false};
    std::atomic<bool> console{false};
    std::atomic<bool> running{true};
};

void input_thread_main(InputState& inp);

}
