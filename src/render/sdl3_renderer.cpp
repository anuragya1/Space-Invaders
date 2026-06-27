#include "sdl3_renderer.h"
#include "sdl3_sprites.h"

#include "../core/difficulty.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace si {

namespace {

constexpr Rgba C_BG          { 10,  12,  24, 255};
constexpr Rgba C_STAR        { 90, 110, 140, 255};
constexpr Rgba C_STAR_DIM    { 50,  60,  80, 255};
constexpr Rgba C_PLAYER      { 90, 220, 120, 255};
constexpr Rgba C_PLAYER2     {210, 140, 255, 255};
constexpr Rgba C_BULLET_P    {130, 210, 255, 255};
constexpr Rgba C_BULLET_2    {220, 130, 240, 255};
constexpr Rgba C_BULLET_A    {255, 100, 100, 255};
constexpr Rgba C_SHIELD_FULL {200, 200, 220, 255};
constexpr Rgba C_SHIELD_MED  {180, 160, 140, 255};
constexpr Rgba C_SHIELD_LOW  {130, 110,  90, 255};
constexpr Rgba C_ALIEN_0     {255, 110, 110, 255};
constexpr Rgba C_ALIEN_1     {255, 220, 120, 255};
constexpr Rgba C_ALIEN_2     {130, 220, 140, 255};
constexpr Rgba C_PU_TRIPLE   {220, 120, 240, 255};
constexpr Rgba C_PU_SHIELD   {120, 220, 240, 255};
constexpr Rgba C_PU_RAPID    {240, 230, 120, 255};
constexpr Rgba C_UFO         {255, 180, 240, 255};
constexpr Rgba C_UFO_DOME    {220, 180, 255, 255};
constexpr Rgba C_BOSS_S1     {220, 120, 240, 255};
constexpr Rgba C_BOSS_S2     {255, 110, 110, 255};
constexpr Rgba C_BOSS_S3     {255, 220, 120, 255};
constexpr Rgba C_EXPL        {255, 200,  80, 255};
constexpr Rgba C_HUD_BG      { 20,  24,  40, 255};
constexpr Rgba C_HUD_TXT     {220, 230, 245, 255};
constexpr Rgba C_HUD_DIM     {140, 155, 175, 255};
constexpr Rgba C_FLASH       {255, 230, 140, 255};
constexpr Rgba C_COMBO       {255, 130,  90, 255};
constexpr Rgba C_BLACK       {  0,   0,   0, 255};
constexpr Rgba C_HALO        {120, 220, 240, 200};
constexpr Rgba C_HP_BG       { 40,  40,  50, 255};

constexpr float SHAKE_DECAY        = 28.0f;
constexpr float SHAKE_PLAYER_HIT   = 8.0f;
constexpr float SHAKE_BOSS_HIT     = 3.0f;
constexpr float SHAKE_EXPLOSION    = 1.5f;
constexpr float SHAKE_MAX          = 14.0f;

[[maybe_unused]] Rgba colorFromAlien(const Alien& a) {
    if (a.row == 0) return C_ALIEN_0;
    if (a.row == 1) return C_ALIEN_1;
    return C_ALIEN_2;
}

[[maybe_unused]] Rgba colorFromPU(const PowerUp& p) {
    switch (p.type) {
        case PUType::TRIPLE: return C_PU_TRIPLE;
        case PUType::SHIELD: return C_PU_SHIELD;
        case PUType::RAPID:  return C_PU_RAPID;
        default:             return Rgba{255, 255, 255, 255};
    }
}

Rgba colorFromBoss(const Boss& b) {
    if (b.stage == 1) return C_BOSS_S1;
    if (b.stage == 2) return C_BOSS_S2;
    return C_BOSS_S3;
}

Rgba colorFromShield(char c) {
    if (c == '#') return C_SHIELD_FULL;
    if (c == '+') return C_SHIELD_MED;
    if (c == '.') return C_SHIELD_LOW;
    return Rgba{0, 0, 0, 0};
}

}

SDL3Renderer::SDL3Renderer()
    : shakeRng_(0xBEEF) {
}

void SDL3Renderer::pre_step(const Game& g) {

    prev_.aliens.clear();
    prev_.aliens.reserve(g.aliens.size());
    for (const auto& a : g.aliens) {
        prev_.aliens.push_back(a.pos);
    }
    prev_.player    = g.player.pos;
    prev_.player2   = g.player2.pos;
    prev_.ufoX      = g.ufo.x;
    prev_.ufoActive = g.ufo.active;
    prev_.boss      = Pt(g.boss.x, g.boss.y);
    prev_.bossActive = g.boss.active;
    prev_.valid     = true;
}

void SDL3Renderer::post_step(const Game& g) {

    if (g.explosions.size() > lastExplosionCount_) {
        for (std::size_t i = lastExplosionCount_;
             i < g.explosions.size(); ++i) {
            const auto& e = g.explosions[i];
            particles_.spawn_explosion(e.pos.x, e.pos.y);
        }

        const std::size_t newOnes = g.explosions.size() - lastExplosionCount_;
        if (!reducedMotion_) {
            shakeAmount_ += SHAKE_EXPLOSION * static_cast<float>(newOnes);
        }
    }
    lastExplosionCount_ = g.explosions.size();

    if (g.player.lives < lastPlayerLives_) {
        if (!reducedMotion_) shakeAmount_ += SHAKE_PLAYER_HIT;
        particles_.spawn_explosion(g.player.pos.x, g.player.pos.y,
                                   C_PLAYER.r, C_PLAYER.g, C_PLAYER.b, 24);
    }
    lastPlayerLives_ = g.player.lives;

    if (g.hasP2 && g.player2.lives < lastPlayer2Lives_) {
        if (!reducedMotion_) shakeAmount_ += SHAKE_PLAYER_HIT;
        particles_.spawn_explosion(g.player2.pos.x, g.player2.pos.y,
                                   C_PLAYER2.r, C_PLAYER2.g, C_PLAYER2.b, 24);
    }
    lastPlayer2Lives_ = g.player2.lives;

    if (g.boss.active && lastBossHp_ > 0 && g.boss.hp < lastBossHp_) {
        if (!reducedMotion_) shakeAmount_ += SHAKE_BOSS_HIT;
        Rgba bc = colorFromBoss(g.boss);
        particles_.spawn_spark(g.boss.x, g.boss.y, +1,
                               bc.r, bc.g, bc.b, 10);

        bossHitFlash_ = 1.0f;
    }
    lastBossHp_ = g.boss.active ? g.boss.hp : 0;

    if (g.ufo.active && !lastUfoActive_) {
        ufoBannerTimer_ = 1.2f;
    }
    lastUfoActive_ = g.ufo.active;

    if (g.boss.active && !lastBossActive_) {
        bossIntroTimer_ = 1.5f;
        bossMaxHpSeen_  = g.boss.maxHp;
    } else if (!g.boss.active) {
        bossMaxHpSeen_ = 0;
    } else if (g.boss.maxHp > bossMaxHpSeen_) {
        bossMaxHpSeen_ = g.boss.maxHp;
    }
    lastBossActive_ = g.boss.active;

    if (shakeAmount_ > SHAKE_MAX) shakeAmount_ = SHAKE_MAX;
}

void SDL3Renderer::tick_render(float dtSec) {

    particles_.update(dtSec);

    renderTime_ += dtSec;

    if (reducedMotion_) {
        shakeAmount_ = 0.0f;
        shakeOffX_ = 0.0f;
        shakeOffY_ = 0.0f;
    }

    if (shakeAmount_ > 0.0f) {
        shakeAmount_ -= SHAKE_DECAY * dtSec;
        if (shakeAmount_ < 0.0f) shakeAmount_ = 0.0f;
    }

    if (shakeAmount_ > 0.01f) {
        std::uniform_real_distribution<float> d(-shakeAmount_, shakeAmount_);
        shakeOffX_ = d(shakeRng_);
        shakeOffY_ = d(shakeRng_);
    } else {
        shakeOffX_ = 0.0f;
        shakeOffY_ = 0.0f;
    }

    if (ufoBannerTimer_ > 0.0f) {
        ufoBannerTimer_ -= dtSec;
        if (ufoBannerTimer_ < 0.0f) ufoBannerTimer_ = 0.0f;
    }
    if (ufoTrailTimer_ > 0.0f) {
        ufoTrailTimer_ -= dtSec;
        if (ufoTrailTimer_ < 0.0f) ufoTrailTimer_ = 0.0f;
    }

    if (bossIntroTimer_ > 0.0f) {
        bossIntroTimer_ -= dtSec;
        if (bossIntroTimer_ < 0.0f) bossIntroTimer_ = 0.0f;
    }
    if (bossHitFlash_ > 0.0f) {
        bossHitFlash_ -= dtSec * 3.0f;
        if (bossHitFlash_ < 0.0f) bossHitFlash_ = 0.0f;
    }
}

void SDL3Renderer::on_restart(const Game& g) {
    particles_.clear();
    shakeAmount_ = 0.0f;
    shakeOffX_ = shakeOffY_ = 0.0f;
    lastExplosionCount_ = g.explosions.size();
    lastPlayerLives_    = g.player.lives;
    lastPlayer2Lives_   = g.hasP2 ? g.player2.lives : 0;
    lastBossHp_         = g.boss.active ? g.boss.hp : 0;

    lastUfoActive_   = g.ufo.active;
    ufoBannerTimer_  = 0.0f;
    ufoTrailTimer_   = 0.0f;
    lastBossActive_  = g.boss.active;
    bossIntroTimer_  = 0.0f;
    bossHitFlash_    = 0.0f;
    bossMaxHpSeen_   = g.boss.active ? g.boss.maxHp : 0;

    prev_.valid = false;
}

void SDL3Renderer::set_reduced_motion(bool enabled) {
    reducedMotion_ = enabled;
    if (reducedMotion_) {
        shakeAmount_ = 0.0f;
        shakeOffX_ = 0.0f;
        shakeOffY_ = 0.0f;
    }
}

void SDL3Renderer::draw_cell(SDL_Renderer* ren, int cx, int cy, Rgba col,
                              float pad) {
    if (cx < 0 || cx >= W || cy < 0 || cy >= H) return;
    set_col(ren, col);
    fill_rect(ren,
              static_cast<float>(px(cx)) + pad + shakeOffX_,
              static_cast<float>(py(cy)) + pad + shakeOffY_,
              static_cast<float>(TILE) - 2.0f * pad,
              static_cast<float>(TILE) - 2.0f * pad);
}

void SDL3Renderer::draw_cell_f(SDL_Renderer* ren, float cx, float cy, Rgba col,
                                float pad) {
    set_col(ren, col);
    fill_rect(ren,
              px_f(cx) + pad + shakeOffX_,
              py_f(cy) + pad + shakeOffY_,
              static_cast<float>(TILE) - 2.0f * pad,
              static_cast<float>(TILE) - 2.0f * pad);
}

void SDL3Renderer::fill_circle(SDL_Renderer* ren, float cx, float cy, float r) {
    if (r <= 0.0f) return;
    const int rint = static_cast<int>(std::ceil(r));
    for (int dy = -rint; dy <= rint; ++dy) {
        const float fy = static_cast<float>(dy);
        const float disc = r * r - fy * fy;
        if (disc < 0.0f) continue;
        const float dx = std::sqrt(disc);
        SDL_FRect line{ cx - dx, cy + fy, 2.0f * dx, 1.0f };
        SDL_RenderFillRect(ren, &line);
    }
}

void SDL3Renderer::draw_text(SDL_Renderer* ren, const std::string& msg,
                              float x, float y, float scale) {
    if (scale == 1.0f) {
        SDL_RenderDebugText(ren, x, y, msg.c_str());
        return;
    }
    float prevSX = 1.0f, prevSY = 1.0f;
    SDL_GetRenderScale(ren, &prevSX, &prevSY);
    SDL_SetRenderScale(ren, scale, scale);
    SDL_RenderDebugText(ren, x / scale, y / scale, msg.c_str());
    SDL_SetRenderScale(ren, prevSX, prevSY);
}

void SDL3Renderer::draw_text_centered(SDL_Renderer* ren, const std::string& msg,
                                       float cx, float y, float scale) {
    const float w = static_cast<float>(msg.size()) * 8.0f * scale;
    draw_text(ren, msg, cx - w * 0.5f, y, scale);
}

void SDL3Renderer::draw_stars(SDL_Renderer* ren, const std::vector<Star>& stars) {

    for (const auto& s : stars) {
        set_col(ren, (s.sym == '.') ? C_STAR_DIM : C_STAR);
        fill_rect(ren,
                  static_cast<float>(px(s.x) + TILE / 2 - 1),
                  static_cast<float>(py(s.y) + TILE / 2 - 1),
                  2.0f, 2.0f);
    }
}

void SDL3Renderer::draw_alien(SDL_Renderer* ren, const Alien& a,
                               const Pt& prev, float alpha) {
    if (!a.alive) return;
    const float fx = lerp_cell(prev.x, a.pos.x, alpha);
    const float fy = lerp_cell(prev.y, a.pos.y, alpha);
    const float bx = px_f(fx) + shakeOffX_;
    const float by = py_f(fy) + shakeOffY_;

    const Sprite* s = &ALIEN_BOT_F0;
    if (a.row == 0)      s = (a.frame == 0) ? &ALIEN_TOP_F0 : &ALIEN_TOP_F1;
    else if (a.row == 1) s = (a.frame == 0) ? &ALIEN_MID_F0 : &ALIEN_MID_F1;
    else                 s = (a.frame == 0) ? &ALIEN_BOT_F0 : &ALIEN_BOT_F1;

    blit_sprite(ren, *s, bx, by, 1.0f);
}

void SDL3Renderer::draw_bullet(SDL_Renderer* ren, const Bullet& b) {

    if (!b.active) return;
    Rgba c = (b.dir == +1)  ? C_BULLET_A
           : (b.owner == 1) ? C_BULLET_2
                            : C_BULLET_P;
    set_col(ren, c);
    fill_rect(ren,
              static_cast<float>(px(b.pos.x) + TILE / 2 - 1) + shakeOffX_,
              static_cast<float>(py(b.pos.y) + 2) + shakeOffY_,
              2.0f,
              static_cast<float>(TILE - 4));
}

void SDL3Renderer::draw_player(SDL_Renderer* ren, const Player& p,
                                const Pt& prev, float alpha, Rgba col) {
    if (p.lives <= 0) return;
    const float fx = lerp_cell(prev.x, p.pos.x, alpha);
    const float fy = lerp_cell(prev.y, p.pos.y, alpha);
    const float bx = px_f(fx) + shakeOffX_;
    const float by = py_f(fy) + shakeOffY_;

    const Sprite* ship = (col.g > col.b) ? &PLAYER_SHIP_P2 : &PLAYER_SHIP;
    blit_sprite(ren, *ship, bx, by, 1.0f);

    if (p.shielded) {
        const float cx = bx + static_cast<float>(TILE / 2);
        const float cy = by + static_cast<float>(TILE / 2);
        const float r  = static_cast<float>(TILE - 2);
        SDL_BlendMode prevBM = SDL_BLENDMODE_NONE;
        SDL_GetRenderDrawBlendMode(ren, &prevBM);
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

        const float pulse = 0.5f + 0.5f * std::sin(renderTime_ * 6.0f);
        const std::uint8_t alphaB =
            static_cast<std::uint8_t>(80.0f + 80.0f * pulse);
        SDL_SetRenderDrawColor(ren, C_HALO.r, C_HALO.g, C_HALO.b, alphaB);

        for (float ring = r; ring > r - 2.0f; ring -= 0.5f) {
            fill_circle(ren, cx, cy, ring);
        }
        SDL_SetRenderDrawBlendMode(ren, prevBM);
    }
}

void SDL3Renderer::draw_shield(SDL_Renderer* ren, const Shield& s) {
    for (int dy = 0; dy < 2; ++dy) {
        for (int dx = 0; dx < 4; ++dx) {
            char c = s.cells[dy][dx];
            if (c == ' ') continue;
            draw_cell(ren, s.x + dx, s.y + dy, colorFromShield(c), 1.0f);
        }
    }
}

void SDL3Renderer::draw_powerup(SDL_Renderer* ren, const PowerUp& pu) {
    if (!pu.active) return;
    const float bx = static_cast<float>(px(pu.pos.x)) + shakeOffX_;
    const float by = static_cast<float>(py(pu.pos.y)) + shakeOffY_;

    const Sprite* s = &POWERUP_TRIPLE;
    if      (pu.type == PUType::SHIELD) s = &POWERUP_SHIELD;
    else if (pu.type == PUType::RAPID)  s = &POWERUP_RAPID;

    std::uint8_t tintR = 255, tintG = 255, tintB = 255;
    if (pu.life < 30 && (pu.life / 4) % 2 == 0) {

    }
    blit_sprite(ren, *s, bx, by, 1.0f, tintR, tintG, tintB);
}

void SDL3Renderer::draw_ufo(SDL_Renderer* ren, const UFO& u, int prevX,
                             float alpha) {
    if (!u.active) return;

    const float fx = lerp_cell(prevX, u.x, alpha);
    const float bx = px_f(fx);
    const float baseX = bx + shakeOffX_;
    const float baseY = static_cast<float>(py(UFO_Y)) + shakeOffY_;

    SDL_BlendMode prevBM = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(ren, &prevBM);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    const float pulse = 0.5f + 0.5f * std::sin(renderTime_ * 10.0f);
    {
        SDL_SetRenderDrawColor(ren,
            C_UFO.r, C_UFO.g, C_UFO.b,
            static_cast<std::uint8_t>(60.0f + 80.0f * pulse));
        SDL_FRect halo{
            baseX - 4.0f,
            baseY + 2.0f,
            static_cast<float>(TILE * 2 + 8),
            static_cast<float>(TILE - 2)
        };
        SDL_RenderFillRect(ren, &halo);
    }
    SDL_SetRenderDrawBlendMode(ren, prevBM);

    const std::uint8_t tintBoost = static_cast<std::uint8_t>(
        std::min(255.0f, 200.0f + 55.0f * pulse));
    blit_sprite(ren, UFO_SPRITE, baseX, baseY, 1.0f,
                tintBoost, tintBoost, tintBoost);

    if (ufoTrailTimer_ <= 0.0f) {
        ufoTrailTimer_ = 0.06f;
        const int spawnCellX = u.x - u.dir;
        if (spawnCellX >= 0 && spawnCellX < W) {
            particles_.spawn_spark(spawnCellX, UFO_Y, 0,
                                   C_UFO.r, C_UFO.g, C_UFO.b, 3);
        }
    }
}

void SDL3Renderer::draw_boss(SDL_Renderer* ren, const Boss& b,
                              const Pt& prev, float alpha) {
    if (!b.active) return;

    const float fx = lerp_cell(prev.x, b.x, alpha);
    const float fy = lerp_cell(prev.y, b.y, alpha);
    const float bx = px_f(fx - 2);
    const float by = py_f(fy);

    SDL_BlendMode prevBM = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(ren, &prevBM);

    float introOffsetY = 0.0f;
    std::uint8_t alphaByte = 0;
    if (bossIntroTimer_ > 0.0f) {
        if (bossIntroTimer_ > 1.0f) {

            const float t = (bossIntroTimer_ - 1.0f) / 0.5f;
            introOffsetY = -static_cast<float>(HUD_H) * t * 1.5f;
        } else {

            const float t = bossIntroTimer_ / 1.0f;
            alphaByte = static_cast<std::uint8_t>(80.0f + 175.0f * (1.0f - t));
        }
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    }

    blit_sprite(ren, BOSS_SPRITE,
                bx + shakeOffX_,
                by + shakeOffY_ + introOffsetY,
                1.0f,
                255, 255, 255,
                alphaByte);

    if (bossHitFlash_ > 0.0f) {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        const std::uint8_t a = static_cast<std::uint8_t>(140.0f * bossHitFlash_);
        SDL_SetRenderDrawColor(ren, 255, 255, 255, a);
        SDL_FRect r{
            bx + shakeOffX_,
            by + shakeOffY_ + introOffsetY,
            static_cast<float>(BOSS_SPRITE.w),
            static_cast<float>(BOSS_SPRITE.h)
        };
        SDL_RenderFillRect(ren, &r);
    }

    SDL_SetRenderDrawBlendMode(ren, prevBM);

    if (b.maxHp > 0) {
        const float barX = static_cast<float>(WIN_W) * 0.10f;
        const float barW = static_cast<float>(WIN_W) * 0.80f;
        const float barY = static_cast<float>(HUD_H) + 4.0f;
        const float barH = 8.0f;
        const float pct  = static_cast<float>(b.hp)
                         / static_cast<float>(b.maxHp);

        set_col(ren, C_HP_BG);
        fill_rect(ren, barX, barY, barW, barH);

        Rgba fill;
        if      (pct > 0.6f) fill = Rgba{ 90, 220, 120, 255};
        else if (pct > 0.3f) fill = Rgba{255, 220, 120, 255};
        else                 fill = Rgba{255, 110, 110, 255};
        set_col(ren, fill);
        fill_rect(ren, barX, barY, barW * pct, barH);

        char buf[64];
        std::snprintf(buf, sizeof buf, "BOSS  STAGE %d/3", b.stage);
        set_col(ren, fill);
        SDL_RenderDebugText(ren, barX, barY - 12.0f, buf);
    }

    if (bossIntroTimer_ > 0.0f) {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        const float t = bossIntroTimer_ / 1.5f;
        const float pulse = 0.5f + 0.5f
            * std::sin(renderTime_ * 24.0f);
        const std::uint8_t bgA = static_cast<std::uint8_t>(180.0f * t);
        const std::uint8_t txA = static_cast<std::uint8_t>(
            std::min(255.0f, 200.0f * t + 55.0f * pulse * t));

        SDL_SetRenderDrawColor(ren, 80, 0, 0, bgA);
        SDL_FRect strip{
            0.0f,
            static_cast<float>(HUD_H) + static_cast<float>(WIN_H - HUD_H) * 0.35f,
            static_cast<float>(WIN_W),
            48.0f
        };
        SDL_RenderFillRect(ren, &strip);

        SDL_SetRenderDrawColor(ren, 255, 110, 110, txA);
        const char* msg = "!! WARNING  BOSS APPROACHING !!";
        const float tw = static_cast<float>(std::strlen(msg)) * 8.0f * 4.0f;
        const float tx = (static_cast<float>(WIN_W) - tw) * 0.5f;
        const float ty = strip.y + 8.0f;
        float pSX = 1.0f, pSY = 1.0f;
        SDL_GetRenderScale(ren, &pSX, &pSY);
        SDL_SetRenderScale(ren, 4.0f, 4.0f);
        SDL_RenderDebugText(ren, tx / 4.0f, ty / 4.0f, msg);
        SDL_SetRenderScale(ren, pSX, pSY);

        SDL_SetRenderDrawBlendMode(ren, prevBM);
    }
}

void SDL3Renderer::draw_explosion(SDL_Renderer* ren, const Expl& e) {
    const float radius = static_cast<float>(TILE)
                       * (1.0f - (static_cast<float>(e.timer) / 6.0f) * 0.5f);
    if (radius <= 0.0f) return;
    Rgba col = C_EXPL;
    col.a = static_cast<std::uint8_t>(255 * e.timer / 6);
    SDL_BlendMode prevBM = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(ren, &prevBM);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    set_col(ren, col);
    fill_circle(ren,
                static_cast<float>(px(e.pos.x) + TILE / 2) + shakeOffX_,
                static_cast<float>(py(e.pos.y) + TILE / 2) + shakeOffY_,
                radius);
    SDL_SetRenderDrawBlendMode(ren, prevBM);
}

void SDL3Renderer::draw_hud(SDL_Renderer* ren, const Game& g) {

    set_col(ren, C_HUD_BG);
    SDL_FRect bg{ 0.0f, 0.0f,
                  static_cast<float>(WIN_W), static_cast<float>(HUD_H) };
    SDL_RenderFillRect(ren, &bg);
    set_col(ren, C_PLAYER);
    SDL_FRect sep{ 0.0f, static_cast<float>(HUD_H - 2),
                   static_cast<float>(WIN_W), 2.0f };
    SDL_RenderFillRect(ren, &sep);

    char buf[256];

    set_col(ren, C_HUD_TXT);
    draw_text(ren, "SPACE INVADERS - Pro Edition", 14.0f, 8.0f, 2.0f);

    std::snprintf(buf, sizeof buf, "SCORE  %06d", g.player.score);
    set_col(ren, C_PLAYER);
    draw_text(ren, buf, 14.0f, 36.0f, 2.0f);

    std::snprintf(buf, sizeof buf, "LEVEL  %02d", g.level());
    set_col(ren, C_HUD_TXT);
    draw_text(ren, buf, 240.0f, 36.0f, 2.0f);

    set_col(ren, C_HUD_DIM);
    draw_text(ren, "LIVES", 400.0f, 36.0f, 2.0f);
    set_col(ren, C_PLAYER);
    for (int i = 0; i < g.player.lives; ++i) {
        SDL_FRect life{ 500.0f + i * 16.0f, 38.0f, 10.0f, 8.0f };
        SDL_RenderFillRect(ren, &life);
    }

    std::snprintf(buf, sizeof buf, "DIFF: %s",
                  difficulty_unchecked(g.diff_idx()).name);
    set_col(ren, C_HUD_DIM);
    draw_text(ren, buf, 14.0f, 64.0f, 1.5f);

    if (g.combo() > 1 && g.combo_timer() > 0) {
        std::snprintf(buf, sizeof buf, "COMBO x%d", g.combo());
        set_col(ren, C_COMBO);
        draw_text(ren, buf, static_cast<float>(WIN_W) - 200.0f, 36.0f, 2.0f);
    }

    set_col(ren, C_HUD_DIM);
    draw_text(ren,
              "A/D move  SPACE shoot  P pause  Q quit  F11 fullscreen",
              static_cast<float>(WIN_W) - 484.0f, 66.0f, 1.0f);

    if (g.player.power != PUType::NONE) {
        const char* name = "";
        Rgba col = C_HUD_TXT;
        switch (g.player.power) {
            case PUType::TRIPLE: name = "TRIPLE"; col = C_PU_TRIPLE; break;
            case PUType::SHIELD: name = "SHIELD"; col = C_PU_SHIELD; break;
            case PUType::RAPID:  name = "RAPID";  col = C_PU_RAPID;  break;
            default: break;
        }
        std::snprintf(buf, sizeof buf, "POWER: %s (%d)",
                      name, g.player.powerTimer);
        set_col(ren, col);
        draw_text(ren, buf, 280.0f, 64.0f, 1.5f);
    }
}

void SDL3Renderer::draw_flash(SDL_Renderer* ren, const Game& g) {
    if (g.flash_timer() <= 0 || g.flash_msg().empty()) return;
    set_col(ren, C_FLASH);
    draw_text_centered(ren, g.flash_msg(),
                       static_cast<float>(WIN_W) * 0.5f,
                       static_cast<float>(HUD_H)
                         + static_cast<float>(WIN_H) * 0.25f,
                       3.0f);
}

void SDL3Renderer::draw_ufo_banner(SDL_Renderer* ren) {
    if (ufoBannerTimer_ <= 0.0f) return;

    float a01;
    if (ufoBannerTimer_ > 1.0f) {

        const float t = (ufoBannerTimer_ - 1.0f) / 0.2f;
        a01 = 1.0f - t;
    } else if (ufoBannerTimer_ < 0.4f) {

        a01 = ufoBannerTimer_ / 0.4f;
    } else {
        a01 = 1.0f;
    }
    if (a01 < 0.0f) a01 = 0.0f;
    if (a01 > 1.0f) a01 = 1.0f;
    const std::uint8_t alphaByte = static_cast<std::uint8_t>(255.0f * a01);

    SDL_BlendMode prevBM = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(ren, &prevBM);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    const float bandH = 28.0f;
    const float bandY = static_cast<float>(HUD_H) + 24.0f;
    SDL_SetRenderDrawColor(ren, 60, 30, 60,
                           static_cast<std::uint8_t>(160.0f * a01));
    SDL_FRect band{
        0.0f, bandY,
        static_cast<float>(WIN_W), bandH
    };
    SDL_RenderFillRect(ren, &band);

    SDL_SetRenderDrawColor(ren, C_UFO.r, C_UFO.g, C_UFO.b, alphaByte);
    const char* msg = "UFO!  +200 BONUS";
    const float tw = static_cast<float>(std::strlen(msg)) * 8.0f * 2.0f;
    const float tx = (static_cast<float>(WIN_W) - tw) * 0.5f;
    const float ty = bandY + 6.0f;

    float pSX = 1.0f, pSY = 1.0f;
    SDL_GetRenderScale(ren, &pSX, &pSY);
    SDL_SetRenderScale(ren, 2.0f, 2.0f);
    SDL_RenderDebugText(ren, tx / 2.0f, ty / 2.0f, msg);
    SDL_SetRenderScale(ren, pSX, pSY);

    SDL_SetRenderDrawBlendMode(ren, prevBM);
}

void SDL3Renderer::draw(SDL_Renderer* ren, const Game& g, float alpha) {

    set_col(ren, C_BG);
    SDL_RenderClear(ren);

    if (!prev_.valid) {
        pre_step(g);
    }

    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    draw_stars(ren, g.stars);

    for (const auto& s : g.shields) draw_shield(ren, s);

    for (std::size_t i = 0; i < g.aliens.size(); ++i) {
        const Pt prev = (i < prev_.aliens.size())
                      ? prev_.aliens[i]
                      : g.aliens[i].pos;
        draw_alien(ren, g.aliens[i], prev, alpha);
    }

    for (const auto& pu : g.powerups) draw_powerup(ren, pu);

    draw_ufo(ren, g.ufo, prev_.ufoActive ? prev_.ufoX : g.ufo.x, alpha);

    draw_boss(ren, g.boss,
              prev_.bossActive ? prev_.boss : Pt(g.boss.x, g.boss.y),
              alpha);

    for (const auto& b : g.bullets) draw_bullet(ren, b);

    draw_player(ren, g.player,  prev_.player,  alpha, C_PLAYER);
    if (g.hasP2) draw_player(ren, g.player2, prev_.player2, alpha, C_PLAYER2);

    for (const auto& e : g.explosions) draw_explosion(ren, e);

    particles_.draw(ren);

    draw_hud(ren, g);

    draw_ufo_banner(ren);

    draw_flash(ren, g);
}

void SDL3Renderer::draw_pause_overlay(SDL_Renderer* ren, const Game& g) {
    SDL_BlendMode prev = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(ren, &prev);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 160);
    SDL_FRect dim{ 0.0f, static_cast<float>(HUD_H),
                   static_cast<float>(WIN_W),
                   static_cast<float>(WIN_H - HUD_H) };
    SDL_RenderFillRect(ren, &dim);
    SDL_SetRenderDrawBlendMode(ren, prev);

    const float cx = static_cast<float>(WIN_W) * 0.5f;
    const float midY = static_cast<float>(HUD_H)
                     + static_cast<float>(WIN_H - HUD_H) * 0.5f;

    set_col(ren, C_PLAYER);
    draw_text_centered(ren, "PAUSED", cx, midY - 60.0f, 6.0f);

    char buf[256];
    std::snprintf(buf, sizeof buf, "Score: %d   Level: %d   Aliens left: %d",
                  g.player.score, g.level(), g.alien_count_alive());
    set_col(ren, C_HUD_TXT);
    draw_text_centered(ren, buf, cx, midY + 20.0f, 2.0f);

    set_col(ren, C_HUD_DIM);
    draw_text_centered(ren, "Press P to resume", cx, midY + 60.0f, 1.5f);
}

void SDL3Renderer::draw_game_over(SDL_Renderer* ren, const Game& g) {
    SDL_BlendMode prev = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(ren, &prev);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 200);
    SDL_FRect dim{ 0.0f, 0.0f,
                   static_cast<float>(WIN_W), static_cast<float>(WIN_H) };
    SDL_RenderFillRect(ren, &dim);
    SDL_SetRenderDrawBlendMode(ren, prev);

    const float cx = static_cast<float>(WIN_W) * 0.5f;
    const float cy = static_cast<float>(WIN_H) * 0.5f;

    set_col(ren, C_BULLET_A);
    draw_text_centered(ren, "GAME OVER", cx, cy - 70.0f, 7.0f);

    char buf[256];
    std::snprintf(buf, sizeof buf, "Final score: %d   Level reached: %d",
                  g.player.score, g.level());
    set_col(ren, C_HUD_TXT);
    draw_text_centered(ren, buf, cx, cy + 10.0f, 2.0f);

    set_col(ren, C_HUD_DIM);
    draw_text_centered(ren, "Press R to restart   ESC to quit",
                       cx, cy + 50.0f, 1.5f);
}

}
