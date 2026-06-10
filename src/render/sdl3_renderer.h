// sdl3_renderer.h - renders Game state into an SDL3 window.
//
// The renderer is tile-based: every cell in the 70x26 playfield maps to a fixed-size
// pixel block. Default TILE = 16 → playfield 1120x416, HUD 1120x96
// on top, full window 1120x512.
//
// At runtime we install logical-presentation letterbox so the window
// can be resized or made fullscreen and the playfield scales cleanly
// (preserving aspect ratio, black bars on the edges).
//
// We use SDL3's built-in SDL_RenderDebugText (8x8 ASCII glyphs baked
// into the library). No SDL_ttf, no font files. Scaled up via
// SDL_SetRenderScale for HUD title and overlays.
//
// Game logic ticks at 12.5fps (FRAME_MS = 80ms) but we render at vsync
// (~60fps). To avoid the cell-by-cell snap, we interpolate each
// entity's previous-tick position towards its current position based
// on `alpha = elapsed / tick_interval` clamped 0..1.
//
//   - main loop calls pre_step() *before* invoking game.step_pub()
//   - pre_step() captures the current state into prev_*
//   - main loop calls post_step() *after* invoking step_pub()
//   - post_step() compares prev_ vs current to spawn particles for
//     anything that just blew up, and updates the screen-shake amount
//   - draw(... alpha) interpolates between prev_ and current using alpha
//
// On any new explosion we kick `shakeAmount_` up. It decays each frame.
// The playfield draws with a small random pixel offset proportional to
// shakeAmount_. HUD draws WITHOUT shake -- a wobbling HUD looks broken.
//
// `particles_` is an internal ParticleSystem that emits debris bursts
// when explosions appear and sparks when bullets hit. Renderer-only
// state; never touches Game / replay / save.
#pragma once

#include "../core/constants.h"
#include "../core/entities.h"
#include "../game/game.h"
#include "sdl3_interp.h"
#include "sdl3_particles.h"

#include <SDL3/SDL.h>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace si {

// A 32-bit RGBA color in SDL byte order (r, g, b, a).
struct Rgba {
    std::uint8_t r, g, b, a;
};

class SDL3Renderer {
public:
    // Tile pixel size and HUD height. The window dimensions are derived:
    //   width  = W * TILE
    //   height = H * TILE + HUD_H
    static constexpr int TILE  = 16;
    static constexpr int HUD_H = 96;

    static constexpr int WIN_W = W * TILE;
    static constexpr int WIN_H = H * TILE + HUD_H;

    SDL3Renderer();

    // Interpolation hooks called around each simulation tick.
    // Capture the current entity positions into the "prev" snapshot.
    // Call this *immediately before* game.step_pub() in the main loop.
    void pre_step(const Game& g);

    // Update post-step renderer state: spawn particles for any new
    // explosions, kick shake if needed. Call this *immediately after*
    // game.step_pub().
    void post_step(const Game& g);

    // Advance time-based renderer state (particles, screen shake decay)
    // by dtSec seconds. Call every frame, before draw().
    void tick_render(float dtSec);

    // Notify the renderer that the game was restarted (clears particles
    // and resets prev snapshot to current).
    void on_restart(const Game& g);

    // Accessibility: disable screen-shake offsets while keeping normal
    // gameplay, particles, interpolation, and HUD rendering intact.
    void set_reduced_motion(bool enabled);
    bool reduced_motion() const { return reducedMotion_; }

    // Draw the world at the current interpolation alpha.
    // Draw the entire game state. alpha in [0..1] selects how far
    // through the current tick we are (0 = at start of tick, 1 = at
    // end). The renderer interpolates entity positions between the
    // prev_ snapshot and the current state.
    void draw(SDL_Renderer* ren, const Game& g, float alpha);

    // Special overlays (game-over, paused). Drawn on top of normal scene.
    void draw_pause_overlay(SDL_Renderer* ren, const Game& g);
    void draw_game_over(SDL_Renderer* ren, const Game& g);

private:
    // Coordinate helpers.
    static int px(int cx) { return cx * TILE; }
    static int py(int cy) { return cy * TILE + HUD_H; }
    static float px_f(float cx) { return cx * static_cast<float>(TILE); }
    static float py_f(float cy) { return cy * static_cast<float>(TILE)
                                       + static_cast<float>(HUD_H); }

    static void set_col(SDL_Renderer* ren, Rgba c) {
        SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
    }
    static void fill_rect(SDL_Renderer* ren, float x, float y, float w, float h) {
        SDL_FRect r{ x, y, w, h };
        SDL_RenderFillRect(ren, &r);
    }

    // Primitive drawing helpers.
    void draw_cell(SDL_Renderer* ren, int cx, int cy, Rgba col, float pad = 1.0f);

    // Same as draw_cell but takes float cell coords (for interpolated
    // entities). pad is in pixels, applied uniformly.
    void draw_cell_f(SDL_Renderer* ren, float cx, float cy, Rgba col, float pad = 1.0f);

    void fill_circle(SDL_Renderer* ren, float cx, float cy, float r);

    void draw_text(SDL_Renderer* ren, const std::string& msg,
                   float x, float y, float scale);
    void draw_text_centered(SDL_Renderer* ren, const std::string& msg,
                            float cx, float y, float scale);

    // Entity drawing helpers. Only moving, stable-identity entities are
    // interpolated.
    void draw_alien(SDL_Renderer* ren, const Alien& a, const Pt& prev, float alpha);
    void draw_bullet(SDL_Renderer* ren, const Bullet& b);
    void draw_player(SDL_Renderer* ren, const Player& p, const Pt& prev,
                     float alpha, Rgba col);
    void draw_shield(SDL_Renderer* ren, const Shield& s);
    void draw_powerup(SDL_Renderer* ren, const PowerUp& pu);
    void draw_ufo(SDL_Renderer* ren, const UFO& u, int prevX, float alpha);
    void draw_boss(SDL_Renderer* ren, const Boss& b, const Pt& prev, float alpha);
    void draw_explosion(SDL_Renderer* ren, const Expl& e);
    void draw_stars(SDL_Renderer* ren, const std::vector<Star>& stars);
    void draw_hud(SDL_Renderer* ren, const Game& g);
    void draw_flash(SDL_Renderer* ren, const Game& g);

    // UFO appearance banner ("UFO!" + "+200 BONUS"), shown briefly when
    // a UFO enters the playfield. Driven by ufoBannerTimer_.
    void draw_ufo_banner(SDL_Renderer* ren);

    // Renderer-owned state.
    InterpSnapshot   prev_;
    ParticleSystem   particles_;

    // Screen shake. shakeAmount_ is in pixels; the playfield draws with
    // a uniform [-shake, +shake] random offset each frame, decaying at
    // SHAKE_DECAY per second.
    float            shakeAmount_ = 0.0f;
    float            shakeOffX_   = 0.0f;
    float            shakeOffY_   = 0.0f;
    bool             reducedMotion_ = false;
    std::mt19937     shakeRng_;

    // Number of explosions in g at last post_step; new ones spawn debris.
    std::size_t      lastExplosionCount_ = 0;
    // Player lives at last post_step; if it drops, kick a stronger shake.
    int              lastPlayerLives_    = 3;
    int              lastPlayer2Lives_   = 0;
    // Boss hp at last post_step; on hp drop, small shake + sparks.
    int              lastBossHp_         = 0;

    // Continuous wall-clock seconds since renderer init. Used for
    // animation phases (UFO pulse, boss-intro fade-in).
    float            renderTime_ = 0.0f;

    // UFO appearance state. Set on the tick the UFO becomes active;
    // counts down in tick_render(). Drives the "UFO!" banner that
    // appears briefly at the top of the playfield.
    bool             lastUfoActive_   = false;
    float            ufoBannerTimer_  = 0.0f;       // seconds remaining
    float            ufoTrailTimer_   = 0.0f;       // throttle trail spawns

    // Boss intro + hit-flash state.
    bool             lastBossActive_  = false;
    float            bossIntroTimer_  = 0.0f;       // 0..1, 1 = just spawned
    float            bossHitFlash_    = 0.0f;       // 0..1, 1 = just hit
    int              bossMaxHpSeen_   = 0;          // for top-of-screen bar
};

} // namespace si
