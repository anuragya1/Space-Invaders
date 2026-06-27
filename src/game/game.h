#pragma once

#include "../core/entities.h"
#include "../core/game_event.h"
#include "../core/rng.h"
#include "../core/difficulty.h"
#include "../core/action.h"
#include "../persistence/save_state.h"
#include "../persistence/stats.h"
#include "../persistence/achievements.h"
#include "../persistence/replay_file.h"
#include "../persistence/level_file.h"
#include "../render/rbuf.h"

#include <atomic>
#include <chrono>
#include <string>
#include <vector>
#include <cstdint>

namespace si {

enum class Mode { SOLO, AI_DEMO, REPLAY, COOP_HOST, COOP_CLIENT };

struct IInputSource;

class Game {
public:

    Game(int diffIdx, Mode m, std::uint32_t seed,
         Stats& s, std::vector<Achievement>& a);
    Game(int diffIdx, Mode m, const LevelFile& level,
         Stats& s, std::vector<Achievement>& a);
    Game(const SaveState& s, Stats& st, std::vector<Achievement>& a);

    SaveState run(IInputSource* p1, IInputSource* p2,
                  std::atomic<bool>* netDead = nullptr);

    SaveState run_headless(IInputSource* p1, IInputSource* p2,
                           std::uint32_t max_ticks = 10000);

    SaveState snap() const;

    const Replay& replay() const { return rec_; }
    const std::vector<GameEvent>& events() const { return events_; }
    int  score()      const { return player.score; }
    int  level()      const { return level_; }
    int  diff_idx()   const { return dIdx_; }
    std::uint32_t tick() const { return tick_; }
    bool quit_flag()  const { return quitFlag_; }

    void set_telemetry_user(const std::string& u) { telemetry_user_ = u; }

    void apply_action_pub(std::uint8_t mask, int player_id) {
        if (player_id == 1 && hasP2) apply_action(mask, player2);
        else                          apply_action(mask, player);
    }
    void step_pub(std::uint8_t m1, std::uint8_t m2) { step(m1, m2); ++tick_; }

    bool         is_game_over() const { return gameOver_; }
    bool         is_paused()    const { return paused_;   }
    int          alien_count_alive() const {
        int n = 0;
        for (const auto& a : aliens) if (a.alive) ++n;
        return n;
    }
    int          combo() const { return combo_; }
    int          combo_timer() const { return comboTimer_; }
    const std::string& flash_msg() const { return flashMsg_; }
    int          flash_timer() const { return flashT_; }
    void         tick_flash_decay() { if (flashT_ > 0) --flashT_; }

    void set_director_modifiers(float shootMul, float moveMul, float dropMul);
    float director_shoot_mult() const { return dirShootMul_; }
    float director_move_mult()  const { return dirMoveMul_;  }
    float director_drop_mult()  const { return dirDropMul_;  }

    Player                player;
    Player                player2;
    bool                  hasP2 = false;
    std::vector<Alien>    aliens;
    std::vector<Bullet>   bullets;
    std::vector<Shield>   shields;
    std::vector<PowerUp>  powerups;
    std::vector<Expl>     explosions;
    std::vector<Star>     stars;
    UFO                   ufo;
    Boss                  boss;
    Mode                  mode;

private:

    void apply_action(std::uint8_t mask, Player& p);
    void step(std::uint8_t m1, std::uint8_t m2);
    void render();

    void init_aliens(bool all = true, const bool grid[AROWS][ACOLS] = nullptr);
    void init_shields(const bool tmpl[2][4] = nullptr);
    void init_stars();
    void start_boss_wave();
    bool is_boss_level() const { return level_ % 5 == 0; }
    void move_aliens();
    void alien_shoot();
    void update_ufo();
    void update_boss();
    void try_drop_pu(int x, int y);
    void apply_pickup(Player& p, PUType t);
    void update_pu();
    void hit_player(Player& p);
    void update_bullets();
    bool all_dead() const;
    void next_level();
    void emit_event(GameEvent e);
    void flash(const std::string& m, int t = 70) { flashMsg_ = m; flashT_ = t; }
    void unlock(const std::string& key);

    void handle_console();

    const Diff&               diff_;
    int                       dIdx_;
    int                       level_;
    bool                      gameOver_     = false;
    bool                      paused_       = false;
    int                       aMoveT_       = 0;
    int                       aMoveD_;
    int                       aShootBase_;
    int                       aDirX_        = 1;
    int                       aShootT_      = 0;
    int                       animT_        = 0;
    int                       animF_        = 0;
    int                       ufoTimer_     = 0;
    int                       combo_        = 0;
    int                       comboTimer_   = 0;
    std::string               flashMsg_;
    int                       flashT_       = 0;
    RBuf                      rbuf_;
    RNG                       rng_;
    std::uint32_t             initialSeed_;
    std::uint32_t             tick_         = 0;
    bool                      recording_    = true;
    Replay                    rec_;
    std::vector<GameEvent>     events_;
    Stats&                    statsRef_;
    std::vector<Achievement>& achRef_;
    int                       levelStartLives_  = 0;
    bool                      lostLifeThisWave_ = false;
    bool                      quitFlag_         = false;

    float                     dirShootMul_      = 1.0f;
    float                     dirMoveMul_       = 1.0f;
    float                     dirDropMul_       = 1.0f;

    int          run_shots_fired   = 0;
    int          run_aliens_killed = 0;
    std::chrono::steady_clock::time_point run_start_time_;

    std::string  telemetry_user_;
    int          level_start_shots_  = 0;
    int          level_start_kills_  = 0;
    int          level_start_deaths_ = 0;
    std::chrono::steady_clock::time_point level_start_time_;
};

}
