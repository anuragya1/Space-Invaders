// game_event.h - explicit simulation events emitted during a tick.
//
// These events are a narrow bridge from simulation to observers such as
// audio, presentation, achievements, telemetry, and the Director. They
// do not replace Game state; they make "what happened this tick" explicit.
#pragma once

#include "entities.h"

#include <cstdint>

namespace si {

enum class GameEventType {
    BulletFired,
    AlienKilled,
    PowerupCollected,
    BossPhaseChanged,
    PlayerHit,
    LevelCleared
};

struct GameEvent {
    GameEventType type;
    std::uint32_t tick = 0;
    int playerId = -1;       // 0=P1, 1=P2, -1=enemy/system
    int x = 0;
    int y = 0;
    int value = 0;           // score, level, hp, or small type-specific value
    int combo = 0;
    PUType powerup = PUType::NONE;
};

} // namespace si
