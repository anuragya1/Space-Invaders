#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace si {

class Game;

enum class Sfx {
    PLAYER_SHOOT,
    ALIEN_SHOOT,
    ALIEN_DIE,
    PLAYER_DIE,
    UFO_LOOP,
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

    bool init();

    void shutdown();

    void play(Sfx s, float volume = 1.0f);

    void observe(const Game& g);

    void on_restart(const Game& g);

    void set_muted(bool m) { muted_ = m; }
    bool muted() const     { return muted_; }

    bool active() const { return stream_ != nullptr; }

    enum class Music {
        NONE,
        MARCH,
        BOSS
    };

    void set_music(Music m, float intensity);

    Music current_music() const { return musicTarget_; }

private:

    static void SDLCALL audio_callback(void* userdata,
                                       SDL_AudioStream* stream,
                                       int additional_amount,
                                       int total_amount);
    void mix_into(SDL_AudioStream* stream, int needed_bytes);

    using Buffer = std::vector<float>;

    static Buffer gen_blip(float freq, float durSec, float decay);
    static Buffer gen_sweep(float startFreq, float endFreq,
                            float durSec, float decay);
    static Buffer gen_noise_burst(float durSec, float decay);
    static Buffer gen_thud(float baseFreq, float durSec);
    static Buffer gen_jingle(const float* freqs, int n,
                             float durPerNote);

    void synthesize_all();

    SDL_AudioStream* stream_ = nullptr;

    std::array<Buffer, static_cast<std::size_t>(Sfx::COUNT)> sounds_;

    struct Voice {
        const Buffer* buf = nullptr;
        std::size_t   pos = 0;
        float         vol = 1.0f;
        bool          active = false;
    };
    static constexpr int VOICE_CAP = 8;
    std::array<Voice, VOICE_CAP> voices_{};

    int           lastScore_       = 0;
    int           lastBullets_     = 0;
    int           lastAliveAliens_ = 0;
    int           lastPlayerLives_ = 0;
    int           lastP2Lives_     = 0;
    bool          lastUfoActive_   = false;
    int           lastBossHp_      = 0;
    bool          lastBossActive_  = false;
    int           lastLevel_       = 0;
    int           lastPowerActive_ = 0;
    bool          lastGameOver_    = false;
    int           ufoLoopCooldown_ = 0;

    bool          muted_ = false;
    bool          firstObserve_ = true;

    Music         musicTarget_      = Music::NONE;
    Music         musicCurrent_     = Music::NONE;
    float         musicIntensity_   = 0.0f;
    float         musicMix_         = 0.0f;

    float         musicPhase_       = 0.0f;
    float         musicBeatTimer_   = 0.0f;
    int           musicNoteIdx_     = 0;
};

}
