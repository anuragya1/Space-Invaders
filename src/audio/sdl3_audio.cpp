// sdl3_audio.cpp - SDL3 audio device, synthesis, mixing, event detection.
#include "sdl3_audio.h"

#include "../core/entities.h"
#include "../game/game.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>

namespace si {

namespace {

// Output format: mono 32-bit float at 22050 Hz. Low sample rate keeps
// the buffers tiny (every sound is well under 1 KB) and is plenty for
// arcade blips - we're not mixing orchestral music.
constexpr int CHANNELS = 1;
constexpr int FREQ     = 22050;

// 2 * PI as float, used by every synthesizer.
constexpr float TWO_PI = 6.28318530717958647692f;

inline float random_in(float lo, float hi) {
    const float t = static_cast<float>(std::rand())
                  / static_cast<float>(RAND_MAX);
    return lo + t * (hi - lo);
}

// Hard-clip protection - sums of many voices can occasionally exceed
// [-1, 1]; soft-clip with tanh-style fold instead of hard clipping.
inline float soft_clip(float x) {
    if (x >  1.0f) return  1.0f - 0.1f * (x - 1.0f) / (x);
    if (x < -1.0f) return -1.0f - 0.1f * (x + 1.0f) / (x);
    return x;
}

} // namespace

// =========================================================================
// Synthesis
// =========================================================================

AudioSystem::Buffer AudioSystem::gen_blip(float freq, float durSec, float decay) {
    const int n = static_cast<int>(durSec * FREQ);
    Buffer b;
    b.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        const float t   = static_cast<float>(i) / static_cast<float>(FREQ);
        const float env = std::exp(-decay * t);
        // Square-ish wave by adding the odd harmonics; sounds more
        // "arcade-y" than a pure sine.
        const float s = std::sin(TWO_PI * freq * t)
                      + 0.33f * std::sin(TWO_PI * 3.0f * freq * t)
                      + 0.20f * std::sin(TWO_PI * 5.0f * freq * t);
        b.push_back(0.45f * env * s);
    }
    return b;
}

AudioSystem::Buffer AudioSystem::gen_sweep(float startFreq, float endFreq,
                                           float durSec, float decay) {
    const int n = static_cast<int>(durSec * FREQ);
    Buffer b;
    b.reserve(static_cast<std::size_t>(n));
    float phase = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t   = static_cast<float>(i) / static_cast<float>(FREQ);
        const float u   = t / durSec;
        const float f   = startFreq + (endFreq - startFreq) * u;
        const float env = std::exp(-decay * t);
        phase += TWO_PI * f / static_cast<float>(FREQ);
        b.push_back(0.45f * env * std::sin(phase));
    }
    return b;
}

AudioSystem::Buffer AudioSystem::gen_noise_burst(float durSec, float decay) {
    const int n = static_cast<int>(durSec * FREQ);
    Buffer b;
    b.reserve(static_cast<std::size_t>(n));
    // Lowpass-smooth white noise → sounds like a small explosion.
    float prev = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t    = static_cast<float>(i) / static_cast<float>(FREQ);
        const float env  = std::exp(-decay * t);
        const float raw  = random_in(-1.0f, 1.0f);
        const float lp   = 0.7f * prev + 0.3f * raw;
        prev = lp;
        b.push_back(0.5f * env * lp);
    }
    return b;
}

AudioSystem::Buffer AudioSystem::gen_thud(float baseFreq, float durSec) {
    const int n = static_cast<int>(durSec * FREQ);
    Buffer b;
    b.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        const float t   = static_cast<float>(i) / static_cast<float>(FREQ);
        // Pitch drops as it plays (classic "thud" feel).
        const float f   = baseFreq * (1.0f - 0.5f * (t / durSec));
        const float env = std::exp(-3.0f * t);
        b.push_back(0.55f * env * std::sin(TWO_PI * f * t));
    }
    return b;
}

AudioSystem::Buffer AudioSystem::gen_jingle(const float* freqs, int notes,
                                            float durPerNote) {
    const int nPer = static_cast<int>(durPerNote * FREQ);
    Buffer b;
    b.reserve(static_cast<std::size_t>(nPer * notes));
    for (int note = 0; note < notes; ++note) {
        for (int i = 0; i < nPer; ++i) {
            const float t   = static_cast<float>(i) / static_cast<float>(FREQ);
            const float env = std::exp(-8.0f * t);
            b.push_back(0.4f * env * std::sin(TWO_PI * freqs[note] * t));
        }
    }
    return b;
}

void AudioSystem::synthesize_all() {
    using S = std::size_t;
    // Tuning these constants is essentially the entire "sound design"
    // pass. Numbers were picked by ear; tweak to taste.
    sounds_[static_cast<S>(Sfx::PLAYER_SHOOT)] = gen_blip(800.0f,  0.10f, 30.0f);
    sounds_[static_cast<S>(Sfx::ALIEN_SHOOT) ] = gen_blip(280.0f,  0.10f, 24.0f);
    sounds_[static_cast<S>(Sfx::ALIEN_DIE)   ] = gen_noise_burst(0.18f, 12.0f);
    sounds_[static_cast<S>(Sfx::PLAYER_DIE)  ] = gen_noise_burst(0.55f,  4.0f);

    sounds_[static_cast<S>(Sfx::UFO_LOOP)    ] = gen_sweep(450.0f, 520.0f,
                                                          0.20f,  4.0f);
    sounds_[static_cast<S>(Sfx::UFO_HIT)     ] = gen_sweep(900.0f, 100.0f,
                                                          0.35f,  6.0f);

    sounds_[static_cast<S>(Sfx::POWERUP)     ] = gen_sweep(400.0f, 1200.0f,
                                                          0.30f,  4.0f);
    sounds_[static_cast<S>(Sfx::BOSS_HIT)    ] = gen_thud(180.0f, 0.20f);

    {
        const float boss_notes[] = { 523.0f, 659.0f, 784.0f, 1047.0f };
        sounds_[static_cast<S>(Sfx::BOSS_DIE)] = gen_jingle(boss_notes, 4, 0.12f);
    }
    {
        const float lvl_notes[] = { 523.0f, 659.0f, 784.0f };
        sounds_[static_cast<S>(Sfx::LEVEL_UP)] = gen_jingle(lvl_notes, 3, 0.10f);
    }
    {
        const float go_notes[] = { 440.0f, 392.0f, 349.0f, 262.0f };
        sounds_[static_cast<S>(Sfx::GAME_OVER)] = gen_jingle(go_notes, 4, 0.18f);
    }
}

// =========================================================================
// Lifecycle
// =========================================================================

AudioSystem::~AudioSystem() {
    shutdown();
}

bool AudioSystem::init() {
    SDL_AudioSpec spec;
    spec.channels = CHANNELS;
    spec.format   = SDL_AUDIO_F32;
    spec.freq     = FREQ;

    stream_ = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        &AudioSystem::audio_callback,
        this);

    if (!stream_) {
        // Audio open failed - that's fine, play() will silently no-op.
        // The game itself stays fully playable.
        return false;
    }

    synthesize_all();

    SDL_ResumeAudioStreamDevice(stream_);
    return true;
}

void AudioSystem::shutdown() {
    if (stream_) {
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
    }
    for (auto& v : voices_) { v.active = false; v.buf = nullptr; }
    for (auto& b : sounds_) b.clear();
}

// =========================================================================
// Mixer
// =========================================================================

void SDLCALL AudioSystem::audio_callback(void* userdata,
                                          SDL_AudioStream* stream,
                                          int additional_amount,
                                          int /*total_amount*/) {
    static_cast<AudioSystem*>(userdata)->mix_into(stream, additional_amount);
}

void AudioSystem::set_music(Music m, float intensity) {
    musicTarget_ = m;
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
    musicIntensity_ = intensity;
}

namespace {

// Frequencies of the MARCH pattern (descending bassline, A2 -> F2-ish).
constexpr float MARCH_NOTES[] = {
    110.00f,    // A2
    103.83f,    // G#2
     98.00f,    // G2
     92.50f     // F#2 -ish
};
constexpr int MARCH_LEN = 4;

// BOSS pattern: alternating low + mid notes for tension.
constexpr float BOSS_NOTES[] = {
     65.41f,    // C2
     82.41f,    // E2
     65.41f,    // C2
     87.31f,    // F2
     61.74f,    // B1
     77.78f,    // Eb2
     61.74f,    // B1
     82.41f     // E2
};
constexpr int BOSS_LEN = 8;

// Returns 1 for the first half of a phase cycle, -1 for the second.
// This is the simplest possible square-wave oscillator.
inline float square_wave(float phase01) {
    return (phase01 < 0.5f) ? 1.0f : -1.0f;
}

} // namespace

void AudioSystem::mix_into(SDL_AudioStream* stream, int needed_bytes) {
    if (needed_bytes <= 0) return;

    const int needed_samples = needed_bytes / static_cast<int>(sizeof(float));
    if (needed_samples <= 0) return;

    constexpr int   CHUNK = 512;
    constexpr float SR    = 22050.0f;
    constexpr float dt    = 1.0f / SR;
    float tmp[CHUNK];

    // ---- Crossfade housekeeping ----
    // If target differs from current, decay the mix to 0 then swap.
    // Otherwise raise mix toward 1.
    constexpr float CROSSFADE_PER_SEC = 4.0f;   // ~0.25s full crossfade

    int remaining = needed_samples;
    while (remaining > 0) {
        const int n = std::min(remaining, CHUNK);
        std::memset(tmp, 0, sizeof(float) * static_cast<std::size_t>(n));

        if (!muted_) {
            // --- Voice pool (SFX) ---
            for (auto& v : voices_) {
                if (!v.active || !v.buf) continue;
                const Buffer& src = *v.buf;
                int take = std::min(
                    n,
                    static_cast<int>(src.size() - v.pos));
                if (take <= 0) {
                    v.active = false;
                    v.buf = nullptr;
                    continue;
                }
                for (int i = 0; i < take; ++i) {
                    tmp[i] += v.vol * src[v.pos + static_cast<std::size_t>(i)];
                }
                v.pos += static_cast<std::size_t>(take);
                if (v.pos >= src.size()) {
                    v.active = false;
                    v.buf = nullptr;
                }
            }

            // --- Music synthesis ---
            if (musicTarget_ != Music::NONE || musicCurrent_ != Music::NONE) {
                // Crossfade the mix factor based on the target.
                float targetMix = (musicTarget_ == musicCurrent_)
                                ? 1.0f : 0.0f;

                // Pick pattern + beat interval.
                const float* notes = nullptr;
                int          notesN = 0;
                float        beatSec = 0.5f;
                if (musicCurrent_ == Music::MARCH) {
                    notes  = MARCH_NOTES;
                    notesN = MARCH_LEN;
                    // Tempo: slow (0.55s) -> fast (0.18s) as intensity rises.
                    beatSec = 0.55f - 0.37f * musicIntensity_;
                } else if (musicCurrent_ == Music::BOSS) {
                    notes  = BOSS_NOTES;
                    notesN = BOSS_LEN;
                    beatSec = 0.18f;   // always frantic
                }

                if (notes != nullptr && notesN > 0) {
                    if (musicNoteIdx_ >= notesN) musicNoteIdx_ = 0;
                    const float baseFreq = notes[musicNoteIdx_];

                    for (int i = 0; i < n; ++i) {
                        // Beat advance.
                        musicBeatTimer_ += dt;
                        if (musicBeatTimer_ >= beatSec) {
                            musicBeatTimer_ -= beatSec;
                            musicNoteIdx_ = (musicNoteIdx_ + 1) % notesN;
                            musicPhase_ = 0.0f;     // restart phase per note
                                                    // (avoids click artifacts)
                        }
                        // Within the note, envelope: fast attack, slow decay
                        // shaped over the beat. Gives each beat punch.
                        const float t01 = musicBeatTimer_ / beatSec;  // 0..1
                        const float env = std::exp(-3.0f * t01);

                        // Square wave at baseFreq.
                        musicPhase_ += baseFreq * dt;
                        while (musicPhase_ >= 1.0f) musicPhase_ -= 1.0f;
                        const float s = square_wave(musicPhase_);

                        // Crossfade ramp this sample (linear is fine for 0.25s).
                        if (musicMix_ < targetMix) {
                            musicMix_ += CROSSFADE_PER_SEC * dt;
                            if (musicMix_ > targetMix) musicMix_ = targetMix;
                        } else if (musicMix_ > targetMix) {
                            musicMix_ -= CROSSFADE_PER_SEC * dt;
                            if (musicMix_ < targetMix) musicMix_ = targetMix;
                        }

                        // 0.18 amplitude keeps music quieter than SFX, so the
                        // shoot/explode sounds always punch through.
                        tmp[i] += 0.18f * env * s * musicMix_;
                    }

                    // After the chunk, if we were fading out and reached 0,
                    // swap the current track to the target so the next chunk
                    // can fade IN to the new track.
                    if (musicCurrent_ != musicTarget_ && musicMix_ <= 0.001f) {
                        musicCurrent_   = musicTarget_;
                        musicNoteIdx_   = 0;
                        musicBeatTimer_ = 0.0f;
                        musicPhase_     = 0.0f;
                    }
                } else {
                    // No notes to play (Music::NONE current). Drift mix
                    // toward 0 silently.
                    if (musicMix_ > 0.0f) {
                        musicMix_ -= CROSSFADE_PER_SEC * static_cast<float>(n) * dt;
                        if (musicMix_ < 0.0f) musicMix_ = 0.0f;
                    }
                    if (musicMix_ <= 0.001f
                        && musicCurrent_ != musicTarget_) {
                        musicCurrent_ = musicTarget_;
                    }
                }
            }

            // Soft-clip the mixed result.
            for (int i = 0; i < n; ++i) tmp[i] = soft_clip(tmp[i]);
        }

        SDL_PutAudioStreamData(
            stream, tmp,
            static_cast<int>(sizeof(float)) * n);
        remaining -= n;
    }
}

// =========================================================================
// Play (triggers a voice)
// =========================================================================

void AudioSystem::play(Sfx s, float volume) {
    if (!stream_) return;
    const std::size_t idx = static_cast<std::size_t>(s);
    if (idx >= sounds_.size()) return;
    if (sounds_[idx].empty()) return;

    // Find an idle voice; if all are busy, steal the oldest-position
    // (likely the longest-playing one).
    Voice* free = nullptr;
    Voice* oldest = nullptr;
    std::size_t oldestPos = 0;
    for (auto& v : voices_) {
        if (!v.active) { free = &v; break; }
        if (v.pos >= oldestPos) { oldestPos = v.pos; oldest = &v; }
    }
    Voice* slot = free ? free : oldest;
    if (!slot) return;

    slot->buf    = &sounds_[idx];
    slot->pos    = 0;
    slot->vol    = std::clamp(volume, 0.0f, 1.0f);
    slot->active = true;
}

// =========================================================================
// observe() - turn game state diffs into sound triggers
// =========================================================================

namespace {
int count_alive_aliens(const Game& g) {
    int n = 0;
    for (const auto& a : g.aliens) if (a.alive) ++n;
    return n;
}
int count_active_bullets(const Game& g) {
    int n = 0;
    for (const auto& b : g.bullets) if (b.active) ++n;
    return n;
}
} // namespace

void AudioSystem::on_restart(const Game& g) {
    lastScore_       = g.score();
    lastBullets_     = count_active_bullets(g);
    lastAliveAliens_ = count_alive_aliens(g);
    lastPlayerLives_ = g.player.lives;
    lastP2Lives_     = g.hasP2 ? g.player2.lives : 0;
    lastUfoActive_   = g.ufo.active;
    lastBossHp_      = g.boss.active ? g.boss.hp : 0;
    lastBossActive_  = g.boss.active;
    lastLevel_       = g.level();
    lastPowerActive_ = static_cast<int>(g.player.power);
    lastGameOver_    = g.is_game_over();
    ufoLoopCooldown_ = 0;
    firstObserve_    = true;
}

void AudioSystem::observe(const Game& g) {
    if (firstObserve_) {
        on_restart(g);          // initial baseline; no sounds this tick
        firstObserve_ = false;
        return;
    }

    // New bullets fired this tick. Distinguishing player vs alien shots
    // by walking the bullets vector and only counting ones that are new
    // (any active bullet whose count went up). Simpler heuristic: bullets
    // total went up → at least one shot was fired. Then we look at the
    // newest bullets to decide if they're player or alien.
    const int curBullets = count_active_bullets(g);
    if (curBullets > lastBullets_) {
        // Scan all active bullets; for each one with y near the player
        // and direction up, count player shot. Sufficient.
        bool playerShot = false;
        bool alienShot  = false;
        for (const auto& b : g.bullets) {
            if (!b.active) continue;
            // Player bullets go upward (dir == -1); alien bullets go
            // downward (dir == +1). We can't tell which are "new" with
            // certainty, but firing both player and alien on the same
            // tick is rare enough that one sound for the union is fine.
            if (b.dir < 0) playerShot = true;
            else           alienShot  = true;
        }
        if (playerShot) play(Sfx::PLAYER_SHOOT, 0.55f);
        if (alienShot)  play(Sfx::ALIEN_SHOOT,  0.45f);
    }
    lastBullets_ = curBullets;

    // Alien deaths: score went up by a multiple of (3-row)*10, OR the
    // alive count dropped. Use the alive-count drop - simpler.
    const int curAliveAliens = count_alive_aliens(g);
    if (curAliveAliens < lastAliveAliens_) {
        const int diedThisTick = lastAliveAliens_ - curAliveAliens;
        for (int i = 0; i < diedThisTick && i < 3; ++i) {
            // Slight pitch variation for batches so it doesn't sound
            // identical N times.
            play(Sfx::ALIEN_DIE, 0.6f);
        }
    }
    lastAliveAliens_ = curAliveAliens;

    // Player died.
    if (g.player.lives < lastPlayerLives_) {
        play(Sfx::PLAYER_DIE, 0.9f);
    }
    lastPlayerLives_ = g.player.lives;
    if (g.hasP2 && g.player2.lives < lastP2Lives_) {
        play(Sfx::PLAYER_DIE, 0.9f);
    }
    lastP2Lives_ = g.hasP2 ? g.player2.lives : 0;

    // UFO appeared → start looping its sound.
    if (g.ufo.active && !lastUfoActive_) {
        play(Sfx::UFO_LOOP, 0.55f);
        ufoLoopCooldown_ = 3;     // ticks until next loop blip (3 * FRAME_MS)
    } else if (g.ufo.active) {
        if (ufoLoopCooldown_ > 0) {
            --ufoLoopCooldown_;
        } else {
            play(Sfx::UFO_LOOP, 0.4f);
            ufoLoopCooldown_ = 3;
        }
    }
    // UFO died (was active, now isn't, and score went up).
    if (lastUfoActive_ && !g.ufo.active && g.score() > lastScore_) {
        play(Sfx::UFO_HIT, 0.8f);
    }
    lastUfoActive_ = g.ufo.active;

    // Boss damage.
    if (g.boss.active && lastBossActive_ && g.boss.hp < lastBossHp_) {
        play(Sfx::BOSS_HIT, 0.6f);
    }
    // Boss defeated.
    if (lastBossActive_ && !g.boss.active && lastBossHp_ > 0) {
        play(Sfx::BOSS_DIE, 0.85f);
    }
    lastBossHp_     = g.boss.active ? g.boss.hp : 0;
    lastBossActive_ = g.boss.active;

    // Powerup pickup: power state went from NONE to something.
    const int curPower = static_cast<int>(g.player.power);
    if (lastPowerActive_ == static_cast<int>(PUType::NONE)
        && curPower != static_cast<int>(PUType::NONE)) {
        play(Sfx::POWERUP, 0.7f);
    }
    lastPowerActive_ = curPower;

    // Level up: level number increased.
    if (g.level() > lastLevel_) {
        play(Sfx::LEVEL_UP, 0.85f);
    }
    lastLevel_ = g.level();

    // Game over (one-shot on transition).
    if (g.is_game_over() && !lastGameOver_) {
        play(Sfx::GAME_OVER, 1.0f);
    }
    lastGameOver_ = g.is_game_over();

    lastScore_ = g.score();
}

} // namespace si
