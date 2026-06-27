#pragma once

#include <cstdint>

namespace si::action {

inline constexpr std::uint8_t LEFT    = 1u << 0;
inline constexpr std::uint8_t RIGHT   = 1u << 1;
inline constexpr std::uint8_t SHOOT   = 1u << 2;
inline constexpr std::uint8_t PAUSE   = 1u << 3;
inline constexpr std::uint8_t QUIT    = 1u << 4;
inline constexpr std::uint8_t CONSOLE = 1u << 5;

}

namespace si {

struct InputFrame {
    std::uint32_t tick;
    std::uint8_t  p1;
    std::uint8_t  p2;
};

}
