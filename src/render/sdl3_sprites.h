#pragma once

#include <SDL3/SDL.h>

#include <cstdint>

namespace si {

struct SpritePalette {
    SDL_Color colors[16];
};

struct Sprite {
    const char*  data;
    int          w;
    int          h;
    const SpritePalette* palette;
};

void blit_sprite(SDL_Renderer* ren, const Sprite& s,
                  float dstX, float dstY,
                  float px = 1.0f,
                  std::uint8_t tintR = 255,
                  std::uint8_t tintG = 255,
                  std::uint8_t tintB = 255,
                  std::uint8_t alpha = 0);

extern const Sprite ALIEN_TOP_F0;
extern const Sprite ALIEN_TOP_F1;
extern const Sprite ALIEN_MID_F0;
extern const Sprite ALIEN_MID_F1;
extern const Sprite ALIEN_BOT_F0;
extern const Sprite ALIEN_BOT_F1;

extern const Sprite PLAYER_SHIP;
extern const Sprite PLAYER_SHIP_P2;

extern const Sprite UFO_SPRITE;
extern const Sprite BOSS_SPRITE;

extern const Sprite POWERUP_TRIPLE;
extern const Sprite POWERUP_SHIELD;
extern const Sprite POWERUP_RAPID;

}
