// action.h - per-tick input encoding.
//
// Every input source (keyboard, AI, replay, network peer) produces a
// 1-byte action mask each tick. This is the shared primitive that
// powers both the replay format and the network protocol.
#pragma once

#include <cstdint>

namespace si::action {

inline constexpr std::uint8_t LEFT    = 1u << 0;
inline constexpr std::uint8_t RIGHT   = 1u << 1;
inline constexpr std::uint8_t SHOOT   = 1u << 2;
inline constexpr std::uint8_t PAUSE   = 1u << 3;
inline constexpr std::uint8_t QUIT    = 1u << 4;
inline constexpr std::uint8_t CONSOLE = 1u << 5;  // open debug console (~)

} // namespace si::action

namespace si {

// One frame of recorded / transmitted input.
struct InputFrame {
    std::uint32_t tick;
    std::uint8_t  p1;
    std::uint8_t  p2;
};

} // namespace si
