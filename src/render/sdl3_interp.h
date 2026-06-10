// sdl3_interp.h - snapshots used for smooth SDL3 motion.
//
// The simulation moves in integer cells at 12.5 fps. SDL3 renders much
// more often than that, so drawing raw cell positions would make the
// player and aliens jump from cell to cell. The renderer fixes that by
// remembering where selected entities were before the current logic tick
// and interpolating toward where they are now:
//
//     rendered_position = prev + (curr - prev) * alpha
//
// This stays entirely on the renderer side. Game does not need to know
// interpolation exists.
//
// Snapshots are taken just before Game::step_pub(). After step_pub(),
// current Game positions are the end of that tick. The renderer then
// blends from previous to current while the next tick accumulates.
#pragma once

#include "../core/entities.h"

#include <cstddef>
#include <vector>

namespace si {

// Snapshot of relevant entity positions at a tick boundary. We only
// snapshot what the renderer actually interpolates - everything else
// (alive flags, sprite frame numbers) is read live from Game.
//
// Vector indices are used as identity for aliens. The Game never
// reorders the aliens vector and never deletes entries (kills just
// flip alive=false), so the index is stable across ticks.
struct InterpSnapshot {
    std::vector<Pt> aliens;     // [i] = position of g.aliens[i] last tick

    Pt   player {};             // P1 position last tick
    Pt   player2{};             // P2 position last tick (if hasP2)

    int  ufoX       = 0;        // UFO x last tick (y is constant = UFO_Y)
    bool ufoActive  = false;

    Pt   boss       {};         // Boss position last tick
    bool bossActive = false;

    // False until we've taken at least one snapshot. The renderer treats
    // this as "no interp this frame; draw at current positions."
    bool valid = false;
};

// Linear interpolate between two integer cell coordinates by alpha
// (0 -> prev, 1 -> curr). Returns a float cell coordinate.
inline float lerp_cell(int prev, int curr, float alpha) {
    const float p = static_cast<float>(prev);
    const float c = static_cast<float>(curr);
    return p + (c - p) * alpha;
}

} // namespace si
