// game.cpp - constructors, run loop, action application.
#include "game.h"
#include "../input/input_source.h"
#include "../platform/platform.h"
#include "../core/colors.h"
#include "../debug/logger.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>

namespace si {

Game::Game(int diffIdx, Mode m, std::uint32_t seed,
           Stats& s, std::vector<Achievement>& a)
    : player (difficulty(diffIdx).lives, 0),
      player2(difficulty(diffIdx).lives, 1),
      mode(m),
      diff_(difficulty(diffIdx)),
      dIdx_(diffIdx),
      level_(1),
      aMoveD_(difficulty(diffIdx).moveDelay),
      rng_(seed),
      initialSeed_(seed),
      statsRef_(s),
      achRef_(a)
{
    ufoTimer_ = 120 + rng_.range(0, 149);
    hasP2 = (m == Mode::COOP_HOST || m == Mode::COOP_CLIENT);
    if (hasP2) player2.pos.x = W / 2 + 4;
    init_aliens();
    init_shields();
    init_stars();
    levelStartLives_ = player.lives;
    rec_.seed    = seed;
    rec_.diffIdx = diffIdx;
    rec_.modeStr =
        (m == Mode::SOLO)    ? "solo" :
        (m == Mode::AI_DEMO) ? "ai"   :
        (m == Mode::REPLAY)  ? "replay" : "coop";
    ++statsRef_.gamesPlayed;
    run_start_time_   = std::chrono::steady_clock::now();
    level_start_time_ = run_start_time_;
    LOG_INFO("Game ctor: mode=" << rec_.modeStr
             << " diff=" << diffIdx << " seed=" << seed);
}

Game::Game(const SaveState& s, Stats& st, std::vector<Achievement>& a)
    : player (difficulty(s.diffIdx).lives, 0),
      player2(difficulty(s.diffIdx).lives, 1),
      mode(Mode::SOLO),
      diff_(difficulty(s.diffIdx)),
      dIdx_(s.diffIdx),
      level_(s.level),
      aMoveD_(s.mDelay),
      aDirX_(s.alienDirX),
      rng_(s.seed),
      initialSeed_(s.seed),
      statsRef_(st),
      achRef_(a)
{
    ufoTimer_ = 120 + rng_.range(0, 149);
    player.score   = s.score;
    player.lives   = s.lives;
    player.pos.x   = s.playerX;
    init_aliens(false, s.aAlive);
    init_shields();
    init_stars();
    levelStartLives_ = player.lives;
    rec_.seed    = s.seed;
    rec_.diffIdx = s.diffIdx;
    rec_.modeStr = "solo";
    ++statsRef_.gamesPlayed;
    run_start_time_ = std::chrono::steady_clock::now();
    LOG_INFO("Game ctor (resume): lvl=" << s.level << " score=" << s.score);
}

void Game::unlock(const std::string& key) {
    for (auto& a : achRef_)
        if (a.key == key && !a.unlocked) {
            a.unlocked = true;
            flash("** ACHIEVEMENT: " + a.desc, 110);
            LOG_INFO("achievement unlocked: " << key);
        }
}

void Game::apply_action(std::uint8_t m, Player& p) {
    if (paused_ || gameOver_)  return;
    if (p.lives <= 0)          return;
    if (m & action::LEFT)  p.mvL();
    if (m & action::RIGHT) p.mvR();
    if (m & action::SHOOT) {
        int maxB = (p.power == PUType::RAPID) ? 5 : diff_.playerBmax;
        int pb = (int)std::count_if(bullets.begin(), bullets.end(),
                  [&](const Bullet& b) {
                      return b.dir == -1 && b.active && b.owner == p.id;
                  });
        if (pb < maxB) {
            ++statsRef_.shotsFired;
            ++run_shots_fired;
            bullets.emplace_back(p.pos.x, p.pos.y - 1, -1, p.id);
            emit_event(GameEvent{GameEventType::BulletFired, tick_,
                                 p.id, p.pos.x, p.pos.y - 1});
            if (p.power == PUType::TRIPLE) {
                if (p.pos.x > 2)
                    bullets.emplace_back(p.pos.x - 1, p.pos.y - 1, -1, p.id);
                if (p.pos.x < W - 3)
                    bullets.emplace_back(p.pos.x + 1, p.pos.y - 1, -1, p.id);
            }
        }
    }
}

void Game::handle_console() {
    // Pause-and-prompt mini REPL. Useful when testing gameplay scenarios
    // without recompiling or hand-playing up to the exact state.
    // Commands: /spawn ufo | /kill all | /level <n> | /lives <n> | /help
    paused_ = true;
    render();  // make sure the world is on screen
    std::cout << color::BWHITE << "\n  > " << color::RST;
    std::cout.flush();
    std::string line;
    if (!std::getline(std::cin, line)) { paused_ = false; return; }

    if      (line == "/help") {
        flash("/spawn ufo | /kill all | /level N | /lives N | /unpause", 80);
    }
    else if (line == "/spawn ufo") {
        ufo.active = true; ufo.x = 1; ufo.dir = 1;
        flash("UFO spawned");
    }
    else if (line == "/kill all") {
        for (auto& a : aliens) a.alive = false;
        flash("All aliens vaporised");
    }
    else if (line.rfind("/level ", 0) == 0) {
        try { level_ = std::stoi(line.substr(7)); next_level(); }
        catch (...) {}
    }
    else if (line.rfind("/lives ", 0) == 0) {
        try { player.lives = std::stoi(line.substr(7)); }
        catch (...) {}
    }
    paused_ = false;
}

void Game::step(std::uint8_t m1, std::uint8_t m2) {
    events_.clear();
    if (m1 & action::QUIT) { gameOver_ = true; quitFlag_ = true; return; }
    if (m2 & action::QUIT) { gameOver_ = true; quitFlag_ = true; return; }
    if (m1 & action::PAUSE) paused_ = !paused_;
    if (m1 & action::CONSOLE && mode == Mode::SOLO) handle_console();
    if (paused_) return;

    apply_action(m1, player);
    if (hasP2) apply_action(m2, player2);

    if (recording_) rec_.frames.push_back(InputFrame{tick_, m1, m2});

    if (comboTimer_ > 0 && --comboTimer_ == 0) combo_ = 0;
    if (++animT_ >= 8) { animT_ = 0; animF_ ^= 1; }
    if (flashT_ > 0) --flashT_;
    // Director-aware thresholds. mult > 1 -> shorter threshold -> faster
    // alien fire / movement. mult < 1 -> longer threshold -> slower.
    // Floor at 1 so we never divide-by-zero or hang.
    const int moveD_eff  = std::max(1,
        (int)((float)aMoveD_ / dirMoveMul_));
    if (++aMoveT_ >= moveD_eff) { aMoveT_ = 0; move_aliens(); }
    int sd = std::max(5, diff_.shootBase - level_ * 2);
    const int sd_eff = std::max(3,
        (int)((float)sd / dirShootMul_));
    if (++aShootT_ >= sd_eff) { aShootT_ = 0; alien_shoot(); }

    update_ufo();
    update_boss();
    update_bullets();
    update_pu();

    for (auto& e : explosions) --e.timer;
    explosions.erase(std::remove_if(explosions.begin(), explosions.end(),
                    [](const Expl& e) { return e.timer <= 0; }),
                    explosions.end());

    if (boss.active && boss.hp <= 0) boss.active = false;
    if (!boss.active && all_dead() && !is_boss_level()) next_level();
    if (!boss.active && is_boss_level() && all_dead()) next_level();
}

SaveState Game::snap() const {
    SaveState s;
    s.valid     = true;
    s.diffIdx   = dIdx_;
    s.score     = player.score;
    s.lives     = player.lives;
    s.level     = level_;
    s.playerX   = player.pos.x;
    s.alienDirX = aDirX_;
    s.mDelay    = aMoveD_;
    s.seed      = initialSeed_;
    for (int r = 0; r < AROWS; ++r)
        for (int c = 0; c < ACOLS; ++c)
            s.aAlive[r][c] = aliens[r * ACOLS + c].alive;
    return s;
}

SaveState Game::run(IInputSource* p1, IInputSource* p2,
                    std::atomic<bool>* netDead) {
    (void)!std::system(SI_CLEAR_CMD);
    platform::hide_cursor();
    std::cout << "\033[H";

    while (!gameOver_) {
        if (netDead && netDead->load()) {
            flash("** PEER DISCONNECTED **", 40);
            gameOver_ = true;
            break;
        }
        std::uint8_t m1 = p1 ? p1->poll(tick_, *this, 0) : 0;
        std::uint8_t m2 = p2 ? p2->poll(tick_, *this, 1) : 0;
        step(m1, m2);
        render();
        if (paused_) {
            double secs = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - run_start_time_).count();
            int mins = (int)(secs / 60);
            int rem  = (int)secs % 60;
            double acc = run_shots_fired > 0
                ? 100.0 * run_aliens_killed / run_shots_fired
                : 0.0;
            std::cout << color::BWHITE << color::BOLD
                      << "  *** PAUSED - press P to resume ***" << color::RST << '\n'
                      << color::BCYAN
                      << "  This run:  shots=" << run_shots_fired
                      << "   kills="           << run_aliens_killed
                      << "   accuracy="        << std::fixed << std::setprecision(1) << acc << "%"
                      << "   time="            << mins << "m" << std::setw(2) << std::setfill('0') << rem << "s"
                      << std::setfill(' ')
                      << "   combo-best="      << statsRef_.highestCombo
                      << color::RST << '\n';
        }
        std::cout.flush();
        ++tick_;
        platform::sleep_ms(FRAME_MS);
    }

    platform::show_cursor();

    if (quitFlag_) {
        (void)!std::system(SI_CLEAR_CMD);
        std::cout << color::BGREEN
                  << "\n  Game saved. Returning to menu.\n" << color::RST;
        platform::sleep_ms(900);
        return snap();
    }

    (void)!std::system(SI_CLEAR_CMD);
    std::cout << "\n\n";
    if (player.lives <= 0 && (!hasP2 || player2.lives <= 0))
        std::cout << color::BRED << color::BOLD
                  << "  +=============================+\n"
                  << "  |       GAME  OVER            |\n"
                  << "  +=============================+\n" << color::RST;
    else
        std::cout << color::BRED << color::BOLD
                  << "  +=============================+\n"
                  << "  |  ALIENS REACHED EARTH!      |\n"
                  << "  +=============================+\n" << color::RST;
    std::cout << color::BYELLOW
              << "\n  Difficulty : " << diff_.name
              << "\n  Score      : " << player.score;
    if (hasP2) std::cout << "\n  P2 Score   : " << player2.score;
    std::cout << "\n  Level      : " << level_ << '\n' << color::RST;

    statsRef_.totalScore += player.score;
    if (statsRef_.totalScore >= 10000)             unlock("VETERAN_10K");
    if (dIdx_ == 3 && level_ >= 5)                  unlock("NIGHTMARE_WIN");
    if (dIdx_ == 4 && level_ >= 3)                  unlock("ULTRA_NIGHTMARE");

    LOG_INFO("game ended: score=" << player.score
             << " level=" << level_ << " quit=" << quitFlag_);

    SaveState end;
    end.valid = false;
    end.score   = player.score;
    end.level   = level_;
    end.diffIdx = dIdx_;
    return end;
}

bool Game::all_dead() const {
    return std::none_of(aliens.begin(), aliens.end(),
                        [](const Alien& a) { return a.alive; });
}

void Game::emit_event(GameEvent e) {
    e.tick = tick_;
    events_.push_back(e);
}

SaveState Game::run_headless(IInputSource* p1, IInputSource* p2,
                             std::uint32_t max_ticks) {
    // No render, no sleep, no stdout. Just simulate.
    while (!gameOver_ && tick_ < max_ticks) {
        std::uint8_t m1 = p1 ? p1->poll(tick_, *this, 0) : 0;
        std::uint8_t m2 = p2 ? p2->poll(tick_, *this, 1) : 0;
        step(m1, m2);
        ++tick_;
    }
    statsRef_.totalScore += player.score;
    SaveState end;
    end.valid   = false;
    end.score   = player.score;
    end.level   = level_;
    end.diffIdx = dIdx_;
    return end;
}

void Game::set_director_modifiers(float shootMul, float moveMul,
                                   float dropMul) {
    // Keep adaptive difficulty within boring-but-safe bounds. The
    // Director can change pacing, but it should not take over the game.
    auto clamp = [](float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    };
    dirShootMul_ = clamp(shootMul, 0.5f, 1.8f);
    dirMoveMul_  = clamp(moveMul,  0.5f, 1.8f);
    dirDropMul_  = clamp(dropMul,  0.5f, 3.0f);
}

} // namespace si
