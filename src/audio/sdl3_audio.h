// sdl3_audio.h - sound effects for the SDL3 windowed build.
//
// SYNTHESIZED, NOT FILE-LOADED
// ============================
// All sound effects are generated programmatically as PCM samples at
// startup. No .wav files, no asset paths, no SDL_mixer. Arcade-style
// blips and bloops fit perfectly with synthesis - that is how the
// original 1978 Space Invaders made its sounds too.
//
// MIXING
// ======
// Multiple sounds can play simultaneously (e.g. player shooting at the
// same moment an alien dies). We open ONE SDL3 audio stream with a
// callback that mixes a small pool of "voices" (active playing sounds)
// every time SDL3 needs more data.
//
// EVENT DETECTION
// ===============
// Audio triggers are detected from game-state diffs in the main loop,
// the same way Phase 1's renderer detects events for screen shake. The
// Game class is NOT modified - audio observes from outside.
//
// MUTE
// ====
// AudioSystem can be muted at runtime. Mute zeroes the output but keeps
// the device open, so unmute is instant. A mute flag can be persisted
// to si_pro.cfg later (the cfg already has a sound bool).
#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace si {

class Game;  // forward; we observe Game state but never include game.h here

// Each sound effect is a small slot in the catalogue. Names match the
// in-game event so trigger sites read naturally.
enum class Sfx {
    PLAYER_SHOOT,
    ALIEN_SHOOT,
    ALIEN_DIE,
    PLAYER_DIE,
    UFO_LOOP,        // played repeatedly while UFO is on screen
    UFO_HIT,
    POWERUP,
    BOSS_HIT,
    BOSS_DIE,
    LEVEL_UP,
    GAME_OVER,
    COUNT
};

class AudioSystem {
public:
    AudioSystem() = default;
    ~AudioSystem();

    // Open the audio device + synthesize all sound buffers. Returns
    // false if SDL_INIT_AUDIO is unavailable or the device fails to
    // open - in that case all play() calls become silent no-ops.
    // SDL_Init(SDL_INIT_AUDIO) must have already been called once.
    bool init();

    // Close the audio device and free buffers.
    void shutdown();

    // Trigger a one-shot sound. Safe to call from the main thread at
    // any time, including before init() (no-op then). Volume is 0..1.
    void play(Sfx s, float volume = 1.0f);

    // Observe Game state and fire sounds for events that occurred
    // since the previous tick. Mirrors SDL3Renderer::post_step() but
    // for audio. Call AFTER game.step_pub() in the main loop.
    void observe(const Game& g);

    // Reset internal "previous state" trackers. Call when a new game
    // starts so we don't fire a death sound from the previous run.
    void on_restart(const Game& g);

    // Mute / unmute. While muted the mixer fills the buffer with
    // silence but the device stays open.
    void set_muted(bool m) { muted_ = m; }
    bool muted() const     { return muted_; }

    // Was the device successfully opened?
    bool active() const { return stream_ != nullptr; }

    // --- Music ---
    // Background music is generated on the fly inside the audio callback
    // (no pre-baked buffers). The main loop tells us what to play and
    // how intensely; the mixer interpolates between music states for a
    // ~0.3-second crossfade so there's no audible cut.
    enum class Music {
        NONE,       // silence
        MARCH,      // the classic 4-note descending bass loop
        BOSS        // heavier 8-note pattern for boss fights
    };
    //
    //   intensity in [0,1]:
    //     - MARCH: 0 = slow tempo (aliens far), 1 = fast (aliens close)
    //     - BOSS:  ignored, always at "boss" tempo
    void set_music(Music m, float intensity);

    Music current_music() const { return musicTarget_; }

private:
    // SDL audio callback - drains the voice pool and mixes into 'out'.
    // Trampolines into mix_into() on the AudioSystem instance.
    static void SDLCALL audio_callback(void* userdata,
                                       SDL_AudioStream* stream,
                                       int additional_amount,
                                       int total_amount);
    void mix_into(SDL_AudioStream* stream, int needed_bytes);

    // A single sound buffer: mono float32 samples at FREQ Hz.
    using Buffer = std::vector<float>;

    // Synthesis helpers (all populate one Buffer).
    static Buffer gen_blip(float freq, float durSec, float decay);
    static Buffer gen_sweep(float startFreq, float endFreq,
                            float durSec, float decay);
    static Buffer gen_noise_burst(float durSec, float decay);
    static Buffer gen_thud(float baseFreq, float durSec);
    static Buffer gen_jingle(const float* freqs, int n,
                             float durPerNote);

    // Build every Buffer in `sounds_`.
    void synthesize_all();

    // ---- State ----
    SDL_AudioStream* stream_ = nullptr;

    std::array<Buffer, static_cast<std::size_t>(Sfx::COUNT)> sounds_;

    // A "voice" is a sound currently playing back. The mixer adds the
    // voice's samples (scaled by its volume) into the output until the
    // buffer is exhausted, then marks the voice idle. Concurrent sounds
    // = pool size; 8 voices is more than enough for an arcade game.
    struct Voice {
        const Buffer* buf = nullptr;
        std::size_t   pos = 0;
        float         vol = 1.0f;
        bool          active = false;
    };
    static constexpr int VOICE_CAP = 8;
    std::array<Voice, VOICE_CAP> voices_{};

    // Diff state for observe().
    int           lastScore_       = 0;
    int           lastBullets_     = 0;
    int           lastAliveAliens_ = 0;
    int           lastPlayerLives_ = 0;
    int           lastP2Lives_     = 0;
    bool          lastUfoActive_   = false;
    int           lastBossHp_      = 0;
    bool          lastBossActive_  = false;
    int           lastLevel_       = 0;
    int           lastPowerActive_ = 0;       // PUType as int; nonzero = powered
    bool          lastGameOver_    = false;
    int           ufoLoopCooldown_ = 0;       // frames until next UFO loop blip

    bool          muted_ = false;
    bool          firstObserve_ = true;       // skip diffs on first call

    // --- Music synth state (mutated on the audio thread inside the
    // callback; mutex'd reads are not necessary because:
    //   - The main thread sets musicTarget_ + musicIntensity_ via a
    //     plain assignment in set_music(). On x86/ARM, naturally aligned
    //     reads of a Music enum / float are atomic in practice. We don't
    //     need strict happens-before; an occasional stale read is fine
    //     because next callback will see the new value.
    Music         musicTarget_      = Music::NONE;
    Music         musicCurrent_     = Music::NONE;
    float         musicIntensity_   = 0.0f;       // [0,1]
    float         musicMix_         = 0.0f;       // crossfade 0..1
                                                  // (1 = fully at target)
    float         musicPhase_       = 0.0f;       // sample-rate phase
    float         musicBeatTimer_   = 0.0f;       // seconds since last beat
    int           musicNoteIdx_     = 0;          // index into current pattern
};

} // namespace si
