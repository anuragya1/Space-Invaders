// sdl3_particles.h - simple particle system for visual flair.
//
// Used for: explosion debris (when an alien blows up), sparks (when a
// bullet hits the boss).
//
// Particles live in pixel space (computed from the cell coordinates
// passed to spawn_*) and are updated every render frame, not every
// game tick. They are pure eye candy and never influence game state:
// the simulation does not know they exist.
//
// We use a single fixed-capacity ring buffer. When full, new particles
// silently overwrite the oldest. For our scale (a couple of hundred at
// peak) this is more than enough and avoids per-frame allocation.
#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <cstdint>

namespace si {

struct Particle {
    float x  = 0.0f,  y  = 0.0f;     // pixel coords
    float vx = 0.0f,  vy = 0.0f;     // pixels/sec
    float life    = 0.0f;            // seconds remaining; <=0 means dead
    float lifeMax = 1.0f;
    std::uint8_t r = 255, g = 255, b = 255;
    float size = 2.0f;               // pixel size of the particle (square)
};

class ParticleSystem {
public:
    static constexpr int CAP = 512;

    // Cell coordinates → pixel coordinates use the same constants the
    // renderer does (TILE = 16, HUD_H = 96). These are hardcoded here
    // to keep this header independent of the renderer.
    static constexpr int TILE  = 16;
    static constexpr int HUD_H = 96;

    // x, y are CELL coordinates (Pt-style). Internally we convert to
    // pixel coordinates (cell-center) so callers can pass e.pos.x /
    // e.pos.y directly.

    // Outward burst, ~24 particles by default. Used for alien deaths
    // and player deaths (with different colors).
    void spawn_explosion(int cellX, int cellY,
                         std::uint8_t r = 255, std::uint8_t g = 200,
                         std::uint8_t b = 80, int count = 24);

    // Small directional burst, used for boss hits. dirY is +1 (down)
    // or -1 (up); spark velocities are biased that direction.
    void spawn_spark(int cellX, int cellY, int dirY,
                     std::uint8_t r = 255, std::uint8_t g = 180,
                     std::uint8_t b = 120, int count = 8);

    void update(float dtSec);                // advance live particles
    void draw(SDL_Renderer* ren) const;      // alpha-blended squares
    void clear();                            // wipe all particles

private:
    std::array<Particle, CAP> pool_{};
    int head_ = 0;                           // ring-buffer write index

    Particle* alloc_();
};

} // namespace si
