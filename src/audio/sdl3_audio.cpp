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

constexpr int CHANNELS = 1;
constexpr int FREQ     = 22050;

constexpr float TWO_PI = 6.28318530717958647692f;

inline float random_in(float lo, float hi) {
    const float t = static_cast<float>(std::rand())
                  / static_cast<float>(RAND_MAX);
    return lo + t * (hi - lo);
}

inline float soft_clip(float x) {
    if (x >  1.0f) return  1.0f - 0.1f * (x - 1.0f) / (x);
    if (x < -1.0f) return -1.0f - 0.1f * (x + 1.0f) / (x);
    return x;
}

}

AudioSystem::Buffer AudioSystem::gen_blip(float freq, float durSec, float decay) {
    const int n = static_cast<int>(durSec * FREQ);
    Buffer b;
    b.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        const float t   = static_cast<float>(i) / static_cast<float>(FREQ);
        const float env = std::exp(-decay * t);

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

constexpr float MARCH_NOTES[] = {
    110.00f,
    103.83f,
     98.00f,
     92.50f
};
constexpr int MARCH_LEN = 4;

constexpr float BOSS_NOTES[] = {
     65.41f,
     82.41f,
     65.41f,
     87.31f,
     61.74f,
     77.78f,
     61.74f,
     82.41f
};
constexpr int BOSS_LEN = 8;

inline float square_wave(float phase01) {
    return (phase01 < 0.5f) ? 1.0f : -1.0f;
}

}

void AudioSystem::mix_into(SDL_AudioStream* stream, int needed_bytes) {
    if (needed_bytes <= 0) return;

    const int needed_samples = needed_bytes / static_cast<int>(sizeof(float));
    if (needed_samples <= 0) return;

    constexpr int   CHUNK = 512;
    constexpr float SR    = 22050.0f;
    constexpr float dt    = 1.0f / SR;
    float tmp[CHUNK];

    constexpr float CROSSFADE_PER_SEC = 4.0f;

    int remaining = needed_samples;
    while (remaining > 0) {
        const int n = std::min(remaining, CHUNK);
        std::memset(tmp, 0, sizeof(float) * static_cast<std::size_t>(n));

        if (!muted_) {

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

            if (musicTarget_ != Music::NONE || musicCurrent_ != Music::NONE) {

                float targetMix = (musicTarget_ == musicCurrent_)
                                ? 1.0f : 0.0f;

                const float* notes = nullptr;
                int          notesN = 0;
                float        beatSec = 0.5f;
                if (musicCurrent_ == Music::MARCH) {
                    notes  = MARCH_NOTES;
                    notesN = MARCH_LEN;

                    beatSec = 0.55f - 0.37f * musicIntensity_;
                } else if (musicCurrent_ == Music::BOSS) {
                    notes  = BOSS_NOTES;
                    notesN = BOSS_LEN;
                    beatSec = 0.18f;
                }

                if (notes != nullptr && notesN > 0) {
                    if (musicNoteIdx_ >= notesN) musicNoteIdx_ = 0;
                    const float baseFreq = notes[musicNoteIdx_];

                    for (int i = 0; i < n; ++i) {

                        musicBeatTimer_ += dt;
                        if (musicBeatTimer_ >= beatSec) {
                            musicBeatTimer_ -= beatSec;
                            musicNoteIdx_ = (musicNoteIdx_ + 1) % notesN;
                            musicPhase_ = 0.0f;

                        }

                        const float t01 = musicBeatTimer_ / beatSec;
                        const float env = std::exp(-3.0f * t01);

                        musicPhase_ += baseFreq * dt;
                        while (musicPhase_ >= 1.0f) musicPhase_ -= 1.0f;
                        const float s = square_wave(musicPhase_);

                        if (musicMix_ < targetMix) {
                            musicMix_ += CROSSFADE_PER_SEC * dt;
                            if (musicMix_ > targetMix) musicMix_ = targetMix;
                        } else if (musicMix_ > targetMix) {
                            musicMix_ -= CROSSFADE_PER_SEC * dt;
                            if (musicMix_ < targetMix) musicMix_ = targetMix;
                        }

                        tmp[i] += 0.18f * env * s * musicMix_;
                    }

                    if (musicCurrent_ != musicTarget_ && musicMix_ <= 0.001f) {
                        musicCurrent_   = musicTarget_;
                        musicNoteIdx_   = 0;
                        musicBeatTimer_ = 0.0f;
                        musicPhase_     = 0.0f;
                    }
                } else {

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

            for (int i = 0; i < n; ++i) tmp[i] = soft_clip(tmp[i]);
        }

        SDL_PutAudioStreamData(
            stream, tmp,
            static_cast<int>(sizeof(float)) * n);
        remaining -= n;
    }
}

void AudioSystem::play(Sfx s, float volume) {
    if (!stream_) return;
    const std::size_t idx = static_cast<std::size_t>(s);
    if (idx >= sounds_.size()) return;
    if (sounds_[idx].empty()) return;

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
}

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
        on_restart(g);
        firstObserve_ = false;
        return;
    }

    bool sawBulletFired = false;
    bool sawAlienKilled = false;
    bool sawPlayerHit = false;
    bool sawPowerup = false;
    bool sawLevelClear = false;
    for (const auto& e : g.events()) {
        switch (e.type) {
            case GameEventType::BulletFired:
                sawBulletFired = true;
                if (e.playerId >= 0) play(Sfx::PLAYER_SHOOT, 0.55f);
                else                 play(Sfx::ALIEN_SHOOT,  0.45f);
                break;
            case GameEventType::AlienKilled:
                sawAlienKilled = true;
                play(Sfx::ALIEN_DIE, 0.6f);
                break;
            case GameEventType::PlayerHit:
                sawPlayerHit = true;
                if (e.value > 0) play(Sfx::PLAYER_DIE, 0.9f);
                break;
            case GameEventType::PowerupCollected:
                sawPowerup = true;
                play(Sfx::POWERUP, 0.7f);
                break;
            case GameEventType::LevelCleared:
                sawLevelClear = true;
                play(Sfx::LEVEL_UP, 0.85f);
                break;
            default:
                break;
        }
    }

    const int curBullets = count_active_bullets(g);
    if (!sawBulletFired && curBullets > lastBullets_) {

        bool playerShot = false;
        bool alienShot  = false;
        for (const auto& b : g.bullets) {
            if (!b.active) continue;

            if (b.dir < 0) playerShot = true;
            else           alienShot  = true;
        }
        if (playerShot) play(Sfx::PLAYER_SHOOT, 0.55f);
        if (alienShot)  play(Sfx::ALIEN_SHOOT,  0.45f);
    }
    lastBullets_ = curBullets;

    const int curAliveAliens = count_alive_aliens(g);
    if (!sawAlienKilled && curAliveAliens < lastAliveAliens_) {
        const int diedThisTick = lastAliveAliens_ - curAliveAliens;
        for (int i = 0; i < diedThisTick && i < 3; ++i) {

            play(Sfx::ALIEN_DIE, 0.6f);
        }
    }
    lastAliveAliens_ = curAliveAliens;

    if (!sawPlayerHit && g.player.lives < lastPlayerLives_) {
        play(Sfx::PLAYER_DIE, 0.9f);
    }
    lastPlayerLives_ = g.player.lives;
    if (!sawPlayerHit && g.hasP2 && g.player2.lives < lastP2Lives_) {
        play(Sfx::PLAYER_DIE, 0.9f);
    }
    lastP2Lives_ = g.hasP2 ? g.player2.lives : 0;

    if (g.ufo.active && !lastUfoActive_) {
        play(Sfx::UFO_LOOP, 0.55f);
        ufoLoopCooldown_ = 3;
    } else if (g.ufo.active) {
        if (ufoLoopCooldown_ > 0) {
            --ufoLoopCooldown_;
        } else {
            play(Sfx::UFO_LOOP, 0.4f);
            ufoLoopCooldown_ = 3;
        }
    }

    if (lastUfoActive_ && !g.ufo.active && g.score() > lastScore_) {
        play(Sfx::UFO_HIT, 0.8f);
    }
    lastUfoActive_ = g.ufo.active;

    if (g.boss.active && lastBossActive_ && g.boss.hp < lastBossHp_) {
        play(Sfx::BOSS_HIT, 0.6f);
    }

    if (lastBossActive_ && !g.boss.active && lastBossHp_ > 0) {
        play(Sfx::BOSS_DIE, 0.85f);
    }
    lastBossHp_     = g.boss.active ? g.boss.hp : 0;
    lastBossActive_ = g.boss.active;

    const int curPower = static_cast<int>(g.player.power);
    if (!sawPowerup
        && lastPowerActive_ == static_cast<int>(PUType::NONE)
        && curPower != static_cast<int>(PUType::NONE)) {
        play(Sfx::POWERUP, 0.7f);
    }
    lastPowerActive_ = curPower;

    if (!sawLevelClear && g.level() > lastLevel_) {
        play(Sfx::LEVEL_UP, 0.85f);
    }
    lastLevel_ = g.level();

    if (g.is_game_over() && !lastGameOver_) {
        play(Sfx::GAME_OVER, 1.0f);
    }
    lastGameOver_ = g.is_game_over();

    lastScore_ = g.score();
}

}
