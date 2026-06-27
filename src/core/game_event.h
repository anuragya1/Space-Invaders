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
    int playerId = -1;
    int x = 0;
    int y = 0;
    int value = 0;
    int combo = 0;
    PUType powerup = PUType::NONE;
};

}
