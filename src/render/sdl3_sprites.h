// sdl3_sprites.h - pixel-art sprites for the SDL3 build.
//
// Each sprite is an N-character-per-row ASCII array. Each character
// maps to one color in a small palette (per-sprite-set). A '.' is
// transparent. This is the same trick the 8-bit / 16-bit consoles
// used: hand-laid pixels in source, no external asset files, no PNG
// loaders, no I/O at startup.
//
// Sprite sizes match the renderer's TILE (= 16). An alien is 16x16,
// the UFO is 32x16 (2 cells wide), the boss is 80x32 (5 cells x 2
// cells), the player is 16x16.
//
// blit_sprite() draws the sprite at a pixel coordinate, scaling each
// source pixel to a px*px destination square (= 1.0 by default). For
// each non-'.' character it issues one SDL_RenderFillRect. ~256
// fillrects per 16x16 sprite at scale 1; trivial at modern frame rates.
//
// Tinting is supported - pass a multiplier (1.0 = no change) so the
// renderer can darken / flash sprites for boss hits, shield glow, etc.
#pragma once

#include <SDL3/SDL.h>

#include <cstdint>

namespace si {

// A small color palette - up to 16 named slots per palette.
struct SpritePalette {
    SDL_Color colors[16];   // index 0 is conventionally transparent (a = 0)
};

// One sprite: pointer to ASCII data, dimensions, and the palette to
// look up colors in.
struct Sprite {
    const char*  data;          // (h*w) chars, row-major; '.' = transparent
    int          w;
    int          h;
    const SpritePalette* palette;
};

// Draw a sprite at pixel coords (dstX, dstY). Each source pixel is
// rendered as a px*px filled rectangle (px = 1 means 1:1 with sprite
// pixels; px > 1 enlarges).
//
//   tintR/G/B in [0..255]: multiplied with the palette color (255 = no
//   tint). alpha overrides the per-pixel palette alpha when nonzero.
//   Pass alpha=0 to keep the palette's own alpha.
void blit_sprite(SDL_Renderer* ren, const Sprite& s,
                  float dstX, float dstY,
                  float px = 1.0f,
                  std::uint8_t tintR = 255,
                  std::uint8_t tintG = 255,
                  std::uint8_t tintB = 255,
                  std::uint8_t alpha = 0);

// Sprite catalog, defined in sdl3_sprites.cpp.

// Two-frame animations: even / odd. AnimF in the game flips 0/1 every
// 8 ticks; renderer picks the matching frame.
extern const Sprite ALIEN_TOP_F0;     // top row (3-pointer)
extern const Sprite ALIEN_TOP_F1;
extern const Sprite ALIEN_MID_F0;     // middle row (2-pointer)
extern const Sprite ALIEN_MID_F1;
extern const Sprite ALIEN_BOT_F0;     // bottom row (1-pointer)
extern const Sprite ALIEN_BOT_F1;

extern const Sprite PLAYER_SHIP;
extern const Sprite PLAYER_SHIP_P2;    // green-tinted variant for P2

extern const Sprite UFO_SPRITE;        // 32x16
extern const Sprite BOSS_SPRITE;       // 80x32

extern const Sprite POWERUP_TRIPLE;
extern const Sprite POWERUP_SHIELD;
extern const Sprite POWERUP_RAPID;

} // namespace si
