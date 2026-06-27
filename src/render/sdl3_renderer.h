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

struct Rgba {
    std::uint8_t r, g, b, a;
};

class SDL3Renderer {
public:

    static constexpr int TILE  = 16;
    static constexpr int HUD_H = 96;

    static constexpr int WIN_W = W * TILE;
    static constexpr int WIN_H = H * TILE + HUD_H;

    SDL3Renderer();

    void pre_step(const Game& g);

    void post_step(const Game& g);

    void tick_render(float dtSec);

    void on_restart(const Game& g);

    void set_reduced_motion(bool enabled);
    bool reduced_motion() const { return reducedMotion_; }

    void draw(SDL_Renderer* ren, const Game& g, float alpha);

    void draw_pause_overlay(SDL_Renderer* ren, const Game& g);
    void draw_game_over(SDL_Renderer* ren, const Game& g);

private:

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

    void draw_cell(SDL_Renderer* ren, int cx, int cy, Rgba col, float pad = 1.0f);

    void draw_cell_f(SDL_Renderer* ren, float cx, float cy, Rgba col, float pad = 1.0f);

    void fill_circle(SDL_Renderer* ren, float cx, float cy, float r);

    void draw_text(SDL_Renderer* ren, const std::string& msg,
                   float x, float y, float scale);
    void draw_text_centered(SDL_Renderer* ren, const std::string& msg,
                            float cx, float y, float scale);

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

    void draw_ufo_banner(SDL_Renderer* ren);

    InterpSnapshot   prev_;
    ParticleSystem   particles_;

    float            shakeAmount_ = 0.0f;
    float            shakeOffX_   = 0.0f;
    float            shakeOffY_   = 0.0f;
    bool             reducedMotion_ = false;
    std::mt19937     shakeRng_;

    std::size_t      lastExplosionCount_ = 0;

    int              lastPlayerLives_    = 3;
    int              lastPlayer2Lives_   = 0;

    int              lastBossHp_         = 0;

    float            renderTime_ = 0.0f;

    bool             lastUfoActive_   = false;
    float            ufoBannerTimer_  = 0.0f;
    float            ufoTrailTimer_   = 0.0f;

    bool             lastBossActive_  = false;
    float            bossIntroTimer_  = 0.0f;
    float            bossHitFlash_    = 0.0f;
    int              bossMaxHpSeen_   = 0;
};

}
