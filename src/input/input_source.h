// input_source.h - polymorphic input abstraction.
//
// Every "thing that can produce per-tick input" implements this.
// The Game loop calls poll() twice per tick (once for each player).
// Sources: keyboard, AI, replay, network peer.
#pragma once

#include "../core/action.h"
#include <atomic>
#include <cstdint>

namespace si {

class Game;  // forward

struct IInputSource {
    virtual ~IInputSource() = default;
    virtual std::uint8_t poll(std::uint32_t tick,
                              const Game& g,
                              int playerId) = 0;
};

// Shared atomic state between the input thread (writer) and the game
// thread (reader). One flag per logical action; the input thread sets
// them as bytes arrive and the KeyboardSource consumes them per tick.
struct InputState {
    std::atomic<bool> left   {false};
    std::atomic<bool> right  {false};
    std::atomic<bool> shoot  {false};
    std::atomic<bool> quit   {false};
    std::atomic<bool> pause  {false};
    std::atomic<bool> console{false};
    std::atomic<bool> running{true};
};

// The input thread entry point. Blocks until inp.running becomes false.
void input_thread_main(InputState& inp);

} // namespace si
