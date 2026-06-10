// game_step.cpp - per-tick state updates.
#include "game.h"
#include "../persistence/telemetry.h"
#include "../ui/sound.h"

#include <algorithm>
#include <cmath>

namespace si {

void Game::init_aliens(bool all, const bool grid[AROWS][ACOLS]) {
    aliens.clear();
    for (int r = 0; r < AROWS; ++r)
        for (int c = 0; c < ACOLS; ++c) {
            Alien a(ASTART_X + c * 6, ASTART_Y + r * 2, r);
            a.alive = all ? true : (grid ? grid[r][c] : true);
            aliens.push_back(a);
        }
}

void Game::init_shields() {
    shields.clear();
    int gap = W / 5;
    for (int i = 0; i < 4; ++i)
        shields.emplace_back(gap + i * gap - 2, H - 6);
}

void Game::init_stars() {
    stars.resize(45);
    for (auto& s : stars) {
        s.x = rng_.range(1, W - 2);
        s.y = rng_.range(1, H - 2);
        int k = rng_.range(0, 2);
        s.sym = (k == 0) ? '+' : (k == 1) ? '.' : '`';
    }
}

void Game::start_boss_wave() {
    aliens.clear();
    boss.active     = true;
    boss.x          = W / 2;
    boss.y          = 3;
    boss.dir        = 1;
    boss.maxHp      = 25 + level_ * 5;
    boss.hp         = boss.maxHp;
    boss.moveTimer  = 0;
    boss.shootTimer = 0;
    boss.pattern    = 0;
    boss.stage      = 1;
    flash("!!! BOSS APPROACHES !!!", 100);
}

void Game::move_aliens() {
    if (boss.active) return;
    bool wall = false;
    for (const auto& a : aliens) {
        if (!a.alive) continue;
        int nx = a.pos.x + aDirX_;
        if (nx <= 1 || nx >= W - 2) { wall = true; break; }
    }
    if (wall) {
        aDirX_ = -aDirX_;
        for (auto& a : aliens) if (a.alive) ++a.pos.y;
    } else {
        for (auto& a : aliens) if (a.alive) a.pos.x += aDirX_;
    }
    for (const auto& a : aliens)
        if (a.alive && a.pos.y >= H - 3) gameOver_ = true;
}

void Game::alien_shoot() {
    if (boss.active) return;
    int cur = (int)std::count_if(bullets.begin(), bullets.end(),
              [](const Bullet& b) { return b.dir == 1 && b.active; });
    if (cur >= diff_.alienBmax) return;

    // Find the front-line alien in each column.
    std::vector<int> col2a(W, -1);
    for (int i = 0; i < (int)aliens.size(); ++i) {
        if (!aliens[i].alive) continue;
        int cx = aliens[i].pos.x;
        if (col2a[cx] == -1 || aliens[i].pos.y > aliens[col2a[cx]].pos.y)
            col2a[cx] = i;
    }
    std::vector<int> front;
    for (int c = 0; c < W; ++c) if (col2a[c] != -1) front.push_back(col2a[c]);
    if (front.empty()) return;
    int idx = front[rng_.range(0, (int)front.size() - 1)];
    bullets.emplace_back(aliens[idx].pos.x, aliens[idx].pos.y + 1, +1, -1);
    emit_event(GameEvent{GameEventType::BulletFired, tick_, -1,
                         aliens[idx].pos.x, aliens[idx].pos.y + 1});
}

void Game::update_ufo() {
    if (boss.active) return;
    if (!ufo.active) {
        if (--ufoTimer_ <= 0) {
            ufo.active = true;
            ufo.dir    = rng_.chance(50) ? 1 : -1;
            ufo.x      = (ufo.dir == 1) ? 1 : W - 2;
            ufo.timer  = 0;
        }
        return;
    }
    if (++ufo.timer >= 3) {
        ufo.timer = 0;
        ufo.x += ufo.dir;
        if (ufo.x <= 0 || ufo.x >= W - 1) {
            ufo.active = false;
            ufoTimer_  = 150 + rng_.range(0, 199);
        }
    }
}

void Game::update_boss() {
    if (!boss.active) return;
    int speed = (boss.stage == 1) ? 4 : (boss.stage == 2) ? 3 : 2;
    if (++boss.moveTimer >= speed) {
        boss.moveTimer = 0;
        boss.x += boss.dir;
        if (boss.x <= 3 || boss.x >= W - 4) boss.dir = -boss.dir;
    }
    int sd = std::max(4, 18 - boss.stage * 4);
    if (++boss.shootTimer >= sd) {
        boss.shootTimer = 0;
        switch (boss.pattern) {
            case 0:  // line
                bullets.emplace_back(boss.x, boss.y + 2, +1, -1);
                break;
            case 1:  // spread
                bullets.emplace_back(boss.x,     boss.y + 2, +1, -1);
                bullets.emplace_back(boss.x - 2, boss.y + 2, +1, -1);
                bullets.emplace_back(boss.x + 2, boss.y + 2, +1, -1);
                break;
            case 2: { // aimed at nearest player
                int tx = player.pos.x;
                if (hasP2 && std::abs(player2.pos.x - boss.x) <
                              std::abs(player.pos.x - boss.x))
                    tx = player2.pos.x;
                int off = (tx < boss.x) ? -1 : (tx > boss.x) ? 1 : 0;
                bullets.emplace_back(boss.x + off, boss.y + 2, +1, -1);
                break;
            }
        }
        emit_event(GameEvent{GameEventType::BulletFired, tick_, -1,
                             boss.x, boss.y + 2});
        boss.pattern = (boss.pattern + 1) % 3;
    }
}

void Game::try_drop_pu(int x, int y) {
    // Base 15% chance, scaled by the Director's drop multiplier.
    // Capped at 60% to avoid trivializing the game in dire moments.
    int pct = static_cast<int>(15.0f * dirDropMul_);
    if (pct > 60) pct = 60;
    if (pct < 1)  pct = 1;
    if (rng_.chance(pct)) {
        PUType t = (PUType)(1 + rng_.range(0, 2));
        powerups.emplace_back(x, y, t);
    }
}

void Game::apply_pickup(Player& p, PUType t) {
    p.power      = t;
    p.powerTimer = 220;
    ++statsRef_.powerupsUsed;
    emit_event(GameEvent{GameEventType::PowerupCollected, tick_,
                         p.id, p.pos.x, p.pos.y, 0, 0, t});
    switch (t) {
        case PUType::TRIPLE:
            flash("*** P" + std::to_string(p.id + 1) + " TRIPLE SHOT! ***"); break;
        case PUType::SHIELD:
            p.shielded = true;
            p.shieldHP = 3;
            flash("*** P" + std::to_string(p.id + 1) + " SHIELD ACTIVE! ***"); break;
        case PUType::RAPID:
            flash("*** P" + std::to_string(p.id + 1) + " RAPID FIRE! ***"); break;
        default: break;
    }
}

void Game::update_pu() {
    for (auto& p : powerups) {
        if (!p.active) continue;
        if (--p.life <= 0)         { p.active = false; continue; }
        if (p.pos == player.pos)   { p.active = false; apply_pickup(player,  p.type); }
        else if (hasP2 && p.pos == player2.pos) {
            p.active = false; apply_pickup(player2, p.type);
        }
    }
    powerups.erase(std::remove_if(powerups.begin(), powerups.end(),
                  [](const PowerUp& p) { return !p.active; }), powerups.end());
    for (Player* pl : { &player, &player2 }) {
        if (!hasP2 && pl == &player2) continue;
        if (pl->power != PUType::NONE && --pl->powerTimer <= 0) {
            pl->power    = PUType::NONE;
            pl->shielded = false;
        }
    }
}

void Game::hit_player(Player& p) {
    emit_event(GameEvent{GameEventType::PlayerHit, tick_,
                         p.id, p.pos.x, p.pos.y,
                         p.shielded ? 0 : 1});
    if (p.shielded) {
        if (--p.shieldHP <= 0) {
            p.shielded = false;
            p.power    = PUType::NONE;
        }
        flash("P" + std::to_string(p.id + 1) +
              " shield absorbed! (" + std::to_string(p.shieldHP) + " HP left)");
        return;
    }
    --p.lives;
    ++statsRef_.deaths;
    lostLifeThisWave_ = true;
    explosions.emplace_back(p.pos.x, p.pos.y);
    ui::beep();
    if (p.lives <= 0) {
        if (!hasP2 || (player.lives <= 0 && player2.lives <= 0))
            gameOver_ = true;
        else
            flash("P" + std::to_string(p.id + 1) +
                  " is out! Partner fights on...");
    } else {
        flash("P" + std::to_string(p.id + 1) +
              " life lost! " + std::to_string(p.lives) + " remaining");
    }
}

void Game::update_bullets() {
    for (auto& b : bullets) b.move();

    // Upward bullets (player-fired) -- check against UFO, boss, aliens, shields.
    for (auto& b : bullets) {
        if (!b.active || b.dir != -1) continue;

        // UFO
        if (ufo.active && b.pos.y == UFO_Y && b.pos.x == ufo.x) {
            b.active = false;
            ufo.active = false;
            ufoTimer_  = 150 + rng_.range(0, 199);
            int bonus = 150 * diff_.scoreMult;
            if (b.owner == 1) player2.score += bonus; else player.score += bonus;
            ++statsRef_.ufosKilled;
            if (statsRef_.ufosKilled >= 5) unlock("UFO_HUNTER");
            flash("UFO DESTROYED! +" + std::to_string(bonus));
            explosions.emplace_back(ufo.x, UFO_Y);
            continue;
        }

        // Boss
        if (boss.active &&
            std::abs(b.pos.x - boss.x) <= 2 &&
            b.pos.y >= boss.y && b.pos.y <= boss.y + 1) {
            b.active = false;
            --boss.hp;
            explosions.emplace_back(b.pos.x, b.pos.y);
            if (boss.hp > 0) {
                int oldStage = boss.stage;
                int frac = (boss.hp * 3) / boss.maxHp;
                boss.stage = std::max(1, 3 - frac);
                if (boss.stage != oldStage) {
                    emit_event(GameEvent{GameEventType::BossPhaseChanged,
                                         tick_, -1, boss.x, boss.y,
                                         boss.stage});
                }
            } else {
                int bonus = 500 * diff_.scoreMult * level_;
                if (b.owner == 1) player2.score += bonus; else player.score += bonus;
                ++statsRef_.bossesKilled;
                unlock("BOSS_SLAYER");
                boss.active = false;
                flash("BOSS DEFEATED! +" + std::to_string(bonus), 120);
                ui::beep();
                for (int e = 0; e < 8; ++e)
                    explosions.emplace_back(boss.x + rng_.range(-3, 3),
                                            boss.y + rng_.range(0, 1));
            }
            continue;
        }

        // Aliens
        for (auto& a : aliens) {
            if (!a.alive || !(a.pos == b.pos)) continue;
            a.alive  = false;
            b.active = false;
            ++combo_;
            if (combo_ > statsRef_.highestCombo) statsRef_.highestCombo = combo_;
            comboTimer_ = 40;
            int pts = a.pts() * diff_.scoreMult;
            if (combo_ >= 3) pts = pts * combo_ / 2;
            if (b.owner == 1) player2.score += pts; else player.score += pts;
            ++statsRef_.aliensKilled;
            ++run_aliens_killed;
            emit_event(GameEvent{GameEventType::AlienKilled, tick_,
                                 b.owner, a.pos.x, a.pos.y, pts, combo_});
            unlock("FIRST_BLOOD");
            if (combo_ == 5)  unlock("COMBO_5");
            if (combo_ == 10) unlock("COMBO_10");
            explosions.emplace_back(a.pos.x, a.pos.y);
            try_drop_pu(a.pos.x, a.pos.y);
            if (combo_ >= 5)
                flash("COMBO x" + std::to_string(combo_) +
                      "!  +" + std::to_string(pts));
        }

        // Shields
        for (auto& sh : shields)
            if (sh.hit(b.pos.x, b.pos.y)) { b.active = false; break; }
    }

    // Downward bullets (alien-fired)
    for (auto& b : bullets) {
        if (!b.active || b.dir != +1) continue;
        for (auto& sh : shields)
            if (sh.hit(b.pos.x, b.pos.y)) { b.active = false; break; }
        if (!b.active) continue;
        if (b.pos == player.pos)           { b.active = false; hit_player(player);  continue; }
        if (hasP2 && b.pos == player2.pos) { b.active = false; hit_player(player2); continue; }
    }

    // Bullet-vs-bullet cancellation.
    for (auto& b1 : bullets) {
        if (!b1.active) continue;
        for (auto& b2 : bullets) {
            if (!b2.active || &b1 == &b2) continue;
            if (b1.dir == b2.dir) continue;
            if (b1.pos == b2.pos) { b1.active = b2.active = false; }
        }
    }

    bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                 [](const Bullet& b) { return !b.active; }), bullets.end());
}

void Game::next_level() {
    emit_event(GameEvent{GameEventType::LevelCleared, tick_, -1,
                         0, 0, level_});
    // Emit per-level telemetry BEFORE the level counter advances.
    if (!telemetry_user_.empty()) {
        auto now = std::chrono::steady_clock::now();
        int secs = (int)std::chrono::duration<double>(
            now - level_start_time_).count();
        LevelTelemetry t{
            /*level*/         level_,
            /*shots_fired*/   run_shots_fired   - level_start_shots_,
            /*aliens_killed*/ run_aliens_killed - level_start_kills_,
            /*deaths_this_*/  statsRef_.deaths  - level_start_deaths_,
            /*seconds*/       secs,
            /*combo_max*/     statsRef_.highestCombo,
            /*score_at_end*/  player.score,
            /*diff_idx*/      dIdx_,
        };
        telemetry_append(telemetry_user_, t);
        level_start_shots_  = run_shots_fired;
        level_start_kills_  = run_aliens_killed;
        level_start_deaths_ = statsRef_.deaths;
        level_start_time_   = now;
    }

    if (!lostLifeThisWave_) unlock("PERFECT_WAVE");
    if (hasP2)              unlock("CO_OP_VICTORY");
    ++level_;
    statsRef_.highestLevel = std::max(statsRef_.highestLevel, level_);
    aMoveD_ = std::max(1, aMoveD_ - 1);
    bullets.clear();
    powerups.clear();
    lostLifeThisWave_ = false;
    if (is_boss_level()) start_boss_wave();
    else                  { init_aliens(); aDirX_ = 1; }
    ufoTimer_ = 100 + rng_.range(0, 119);
    flash("=== LEVEL " + std::to_string(level_) +
          " - BRACE YOURSELF ===", 100);
}

} // namespace si
