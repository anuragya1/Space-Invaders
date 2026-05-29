// sdl3_interp.h - position snapshot for smooth-motion interpolation.
//
// The Game engine ticks logic at 12.5 fps (FRAME_MS = 80 ms). Entities
// have integer cell positions that change once per tick. If we render
// those positions raw at 60 fps, motion looks snappy and jerky - the
// player jumps one cell every five render frames.
//
// To smooth this out we keep a snapshot of where every entity *was* on
// the previous logical tick. At render time we have a fractional alpha
// in [0,1] representing how far we are into the current tick
// (alpha = accumulator_ns / tick_ns). We interpolate:
//
//     rendered_position = prev + (curr - prev) * alpha
//
// This adds zero coupling to the Game class - it is purely a render
// concern. Game does not need to know about it.
//
// Snapshots are taken just BEFORE Game::step_pub() so 'prev' captures
// the position at the START of the current tick. After step_pub the
// entity's position field is the END of that same tick. Interpolation
// then smoothly walks the renderer between them as the next tick's
// accumulator fills up.
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
