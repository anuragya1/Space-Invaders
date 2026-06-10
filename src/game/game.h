// game.h - the orchestrator: holds entities, runs ticks, renders.
//
// Driven by IInputSource pointers (one per player). The same loop
// services every mode: solo, AI demo, replay playback, network co-op.
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
    // Constructors.
    Game(int diffIdx, Mode m, std::uint32_t seed,
         Stats& s, std::vector<Achievement>& a);
    Game(const SaveState& s, Stats& st, std::vector<Achievement>& a);

    // Main loop. Caller owns the sources. Returns a snapshot suitable for
    // saving (only meaningful if game was quit, not finished).
    SaveState run(IInputSource* p1, IInputSource* p2,
                  std::atomic<bool>* netDead = nullptr);

    // Headless variant: no render, no sleep, no stdin/stdout. Used by
    // training and tests. Stops at game-over or after max_ticks.
    SaveState run_headless(IInputSource* p1, IInputSource* p2,
                           std::uint32_t max_ticks = 10000);

    // Snapshot of current state (for save-and-quit).
    SaveState snap() const;

    // Accessors used by AI and tests.
    const Replay& replay() const { return rec_; }
    const std::vector<GameEvent>& events() const { return events_; }
    int  score()      const { return player.score; }
    int  level()      const { return level_; }
    int  diff_idx()   const { return dIdx_; }
    std::uint32_t tick() const { return tick_; }
    bool quit_flag()  const { return quitFlag_; }

    // Optional - if non-empty, per-level telemetry rows are written to
    // <user>_curves.csv on each level transition. Handy when balancing
    // difficulty curves or comparing AI runs.
    void set_telemetry_user(const std::string& u) { telemetry_user_ = u; }

    // The terminal entry point uses run() which owns its own loop, sleep,
    // and render. The SDL3 entry point needs to drive the simulation from
    // its own event/draw loop, so we expose the underlying step and
    // input-application methods. These are the same methods run() calls
    // internally; behaviour is identical.
    void apply_action_pub(std::uint8_t mask, int player_id) {
        if (player_id == 1 && hasP2) apply_action(mask, player2);
        else                          apply_action(mask, player);
    }
    void step_pub(std::uint8_t m1, std::uint8_t m2) { step(m1, m2); ++tick_; }

    // Status the windowed loop needs to render and decide game-over.
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

    // External "director" code (in src/director/) watches game state
    // and nudges these multipliers each tick to keep the player in the
    // flow zone. Defaults are 1.0 = no effect, so the terminal build
    // and the unit tests see identical behaviour.
    //
    // Bounds applied internally: shoot/move can range 0.5x to 1.8x,
    // drop chance 0.5x to 3.0x. The setter clamps to safe ranges.
    void set_director_modifiers(float shootMul, float moveMul, float dropMul);
    float director_shoot_mult() const { return dirShootMul_; }
    float director_move_mult()  const { return dirMoveMul_;  }
    float director_drop_mult()  const { return dirDropMul_;  }

    // Public game state read by AI, renderer, and tests.
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
    // Per-tick simulation stages.
    void apply_action(std::uint8_t mask, Player& p);
    void step(std::uint8_t m1, std::uint8_t m2);
    void render();

    void init_aliens(bool all = true, const bool grid[AROWS][ACOLS] = nullptr);
    void init_shields();
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

    // Cheat / console support.
    void handle_console();

    // Internal run state.
    const Diff&               diff_;
    int                       dIdx_;
    int                       level_;
    bool                      gameOver_     = false;
    bool                      paused_       = false;
    int                       aMoveT_       = 0;
    int                       aMoveD_;
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

    // Director AI multipliers, default 1.0 (no effect). See public
    // set_director_modifiers() above.
    float                     dirShootMul_      = 1.0f;
    float                     dirMoveMul_       = 1.0f;
    float                     dirDropMul_       = 1.0f;

    // Per-run counters (reset each game). Used by the pause overlay
    // so the player sees stats for *this* run, not lifetime.
    int          run_shots_fired   = 0;
    int          run_aliens_killed = 0;
    std::chrono::steady_clock::time_point run_start_time_;

    // Per-level snapshots taken at level transition.
    std::string  telemetry_user_;
    int          level_start_shots_  = 0;
    int          level_start_kills_  = 0;
    int          level_start_deaths_ = 0;
    std::chrono::steady_clock::time_point level_start_time_;
};

} // namespace si
