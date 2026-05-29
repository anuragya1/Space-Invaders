// constants.h - playfield dimensions, alien grid, frame timing, network port.
#pragma once

namespace si {

inline constexpr int W         = 70;   // playfield width  (cells)
inline constexpr int H         = 26;   // playfield height (cells)
inline constexpr int AROWS     = 3;    // alien grid rows
inline constexpr int ACOLS     = 11;   // alien grid columns
inline constexpr int ASTART_X  = 3;
inline constexpr int ASTART_Y  = 2;
inline constexpr int UFO_Y     = 1;
inline constexpr int FRAME_MS  = 80;   // ~12.5 fps
inline constexpr int NET_PORT  = 7777;

} // namespace si
