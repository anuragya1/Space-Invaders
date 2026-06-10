// sdl3_audio.h - synthesized sound for the SDL3 build.
//
// The game generates its arcade blips at startup instead of loading
// .wav files. That keeps the project self-contained and fits the visual
// style. The tradeoff is that richer audio would need either more
// synthesis work or a real asset pipeline later.
//
// AudioSystem owns one SDL3 audio stream. The callback mixes a small pool
// of active voices so a shot, explosion, and jingle can overlap.
//
// Sounds are triggered from gameplay events first, with state-diff
// fallback kept for older paths. Audio observes the Game; it should not
// change simulation state.
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

    // Background music is generated in the audio callback. The main loop
    // only tells us the desired music state and intensity; the mixer
    // smooths transitions so menu/game/boss changes do not click.
    enum class Music {
        NONE,       // silence
        MARCH,      // the classic 4-note descending bass loop
        BOSS        // heavier 8-note pattern for boss fights
    };
    // intensity in [0,1]:
    //   MARCH: 0 = slow tempo, 1 = fastest tempo
    //   BOSS: ignored; boss music uses its own tempo
    void set_music(Music m, float intensity);

    Music current_music() const { return musicTarget_; }

private:
    // SDL calls this on the audio thread. It forwards into the owning
    // AudioSystem so the mixer can use instance state.
    static void SDLCALL audio_callback(void* userdata,
                                       SDL_AudioStream* stream,
                                       int additional_amount,
                                       int total_amount);
    void mix_into(SDL_AudioStream* stream, int needed_bytes);

    // A single sound buffer: mono float32 samples at FREQ Hz.
    using Buffer = std::vector<float>;

    // Synthesis helpers. Each returns one ready-to-play sample buffer.
    static Buffer gen_blip(float freq, float durSec, float decay);
    static Buffer gen_sweep(float startFreq, float endFreq,
                            float durSec, float decay);
    static Buffer gen_noise_burst(float durSec, float decay);
    static Buffer gen_thud(float baseFreq, float durSec);
    static Buffer gen_jingle(const float* freqs, int n,
                             float durPerNote);

    // Build every Buffer in `sounds_`.
    void synthesize_all();

    // Audio device and generated SFX buffers.
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

    // Previous simulation state used by observe() fallback detection.
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

    // Music synth state. The main thread writes the target/intensity and
    // the audio callback reads them. A stale read for one callback is
    // harmless; the next buffer will move toward the newest target.
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
