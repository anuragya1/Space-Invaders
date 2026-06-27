#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <cstdint>

namespace si {

struct Particle {
    float x  = 0.0f,  y  = 0.0f;
    float vx = 0.0f,  vy = 0.0f;
    float life    = 0.0f;
    float lifeMax = 1.0f;
    std::uint8_t r = 255, g = 255, b = 255;
    float size = 2.0f;
};

class ParticleSystem {
public:
    static constexpr int CAP = 512;

    static constexpr int TILE  = 16;
    static constexpr int HUD_H = 96;

    void spawn_explosion(int cellX, int cellY,
                         std::uint8_t r = 255, std::uint8_t g = 200,
                         std::uint8_t b = 80, int count = 24);

    void spawn_spark(int cellX, int cellY, int dirY,
                     std::uint8_t r = 255, std::uint8_t g = 180,
                     std::uint8_t b = 120, int count = 8);

    void update(float dtSec);
    void draw(SDL_Renderer* ren) const;
    void clear();

private:
    std::array<Particle, CAP> pool_{};
    int head_ = 0;

    Particle* alloc_();
};

}
