#include "sdl3_particles.h"

#include <SDL3/SDL.h>
#include <cmath>
#include <cstdlib>

namespace si {

namespace {

float frand(float lo, float hi) {
    const float t = static_cast<float>(std::rand())
                  / static_cast<float>(RAND_MAX);
    return lo + t * (hi - lo);
}

float cell_to_pxcx(int cellX) {
    return static_cast<float>(cellX) * static_cast<float>(ParticleSystem::TILE)
         + static_cast<float>(ParticleSystem::TILE) * 0.5f;
}
float cell_to_pxcy(int cellY) {
    return static_cast<float>(cellY) * static_cast<float>(ParticleSystem::TILE)
         + static_cast<float>(ParticleSystem::HUD_H)
         + static_cast<float>(ParticleSystem::TILE) * 0.5f;
}

}

Particle* ParticleSystem::alloc_() {
    Particle* p = &pool_[head_];
    head_ = (head_ + 1) % CAP;
    return p;
}

void ParticleSystem::spawn_explosion(int cellX, int cellY,
                                      std::uint8_t r, std::uint8_t g,
                                      std::uint8_t b, int count) {
    const float px = cell_to_pxcx(cellX);
    const float py = cell_to_pxcy(cellY);
    for (int i = 0; i < count; ++i) {
        Particle* p = alloc_();
        const float angle = frand(0.0f, 6.283185f);
        const float speed = frand(40.0f, 140.0f);
        p->x  = px;
        p->y  = py;
        p->vx = std::cos(angle) * speed;
        p->vy = std::sin(angle) * speed;
        p->life    = frand(0.35f, 0.8f);
        p->lifeMax = p->life;
        p->r = r; p->g = g; p->b = b;
        p->size = frand(1.5f, 3.0f);
    }
}

void ParticleSystem::spawn_spark(int cellX, int cellY, int dirY,
                                  std::uint8_t r, std::uint8_t g,
                                  std::uint8_t b, int count) {
    const float px = cell_to_pxcx(cellX);
    const float py = cell_to_pxcy(cellY);
    const float biasY = (dirY >= 0 ? 1.0f : -1.0f);
    for (int i = 0; i < count; ++i) {
        Particle* p = alloc_();
        const float angle = frand(0.0f, 6.283185f);
        const float speed = frand(20.0f, 70.0f);
        p->x  = px;
        p->y  = py;
        p->vx = std::cos(angle) * speed;

        p->vy = std::sin(angle) * speed + biasY * 30.0f;
        p->life    = frand(0.15f, 0.35f);
        p->lifeMax = p->life;
        p->r = r; p->g = g; p->b = b;
        p->size = frand(1.0f, 2.0f);
    }
}

void ParticleSystem::update(float dtSec) {
    for (auto& p : pool_) {
        if (p.life <= 0.0f) continue;
        p.life -= dtSec;
        if (p.life <= 0.0f) { p.life = 0.0f; continue; }
        p.x  += p.vx * dtSec;
        p.y  += p.vy * dtSec;

        p.vx *= 0.96f;
        p.vy *= 0.96f;
    }
}

void ParticleSystem::draw(SDL_Renderer* ren) const {
    SDL_BlendMode prev = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(ren, &prev);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    for (const auto& p : pool_) {
        if (p.life <= 0.0f) continue;
        const float t = p.life / p.lifeMax;
        const std::uint8_t a = static_cast<std::uint8_t>(255.0f * t);
        SDL_SetRenderDrawColor(ren, p.r, p.g, p.b, a);
        SDL_FRect rect{
            p.x - p.size * 0.5f,
            p.y - p.size * 0.5f,
            p.size, p.size
        };
        SDL_RenderFillRect(ren, &rect);
    }

    SDL_SetRenderDrawBlendMode(ren, prev);
}

void ParticleSystem::clear() {
    for (auto& p : pool_) p.life = 0.0f;
    head_ = 0;
}

}
