# Architecture

This is the short map of how the project fits together. It is not a
class-by-class reference. The code is small enough that the best way to
learn it is still to read the main files.

## Core Idea

Most of the project hangs off one decision:

```text
input source -> action mask -> deterministic Game simulation
```

Keyboard input, SDL3 input, AI, replay playback, network co-op, tests,
and tools all feed small action masks into the same `Game` loop. That is
why replay verification, headless tests, AI runs, and lockstep co-op can
share so much code.

## Main Layers

```text
main.cpp / main_sdl3.cpp
        |
        v
ui / tools / renderer / audio
        |
        v
game/
        |
        v
input/   persistence/   director/
        |
        v
core/
```

`platform/` hides POSIX vs Win32 details. `net/` wraps TCP sockets.
`config/` and `debug/` are used by several layers.

## Directory Map

| Directory | Role |
|---|---|
| `src/core/` | Constants, entities, action masks, RNG, difficulty, events |
| `src/game/` | Main simulation and terminal rendering bridge |
| `src/input/` | Keyboard, SDL3 keyboard, AI, replay, and co-op input |
| `src/render/` | Terminal buffer and SDL3 rendering code |
| `src/audio/` | SDL3 synthesized audio |
| `src/director/` | Adaptive Director AI |
| `src/persistence/` | Saves, stats, achievements, leaderboard, replays, levels, telemetry |
| `src/ui/` | Terminal menus/tools and SDL3 screens |
| `src/net/` | TCP lockstep helpers |
| `src/platform/` | OS-specific terminal/network setup |
| `tests/` | Small C++ test executables |

## Game

`Game` lives in `src/game/game.h` and is implemented across:

- `game.cpp` - construction, run loops, snapshots, input application
- `game_step.cpp` - per-tick gameplay rules
- `game_render.cpp` - terminal rendering

This is not a full ECS. `Game` owns vectors of aliens, bullets, shields,
power-ups, explosions, stars, and boss/UFO state directly. That keeps the
current game easy to follow, but it also means changes to `Game` need
care. If a new system only needs to react to something, prefer events or
an observer-style path instead of adding more direct responsibilities to
`Game`.

## Input Sources

All play modes use `IInputSource`:

```cpp
std::uint8_t poll(std::uint32_t tick, const Game& g, int playerId);
```

The returned byte is an action mask from `src/core/action.h`:

```text
LEFT, RIGHT, SHOOT, PAUSE, QUIT, CONSOLE
```

Current input sources:

| Source | Use |
|---|---|
| `KeyboardSource` | Terminal player input |
| `SDL3Keyboard` | SDL3 player input |
| `AISource` | AI demo, training, tuning |
| `ReplaySource` | Replay playback and verification |
| `CoopSource` | LAN co-op |

This is one of the important extension points. A controller input path,
for example, should produce the same action mask instead of changing the
simulation API.

## Fixed Tick Loop

Gameplay advances in fixed ticks. Rendering may happen at a different
rate, especially in SDL3, but simulation changes happen inside the tick.

The rough order is:

```text
clear events
apply input
record replay frame
update timers
move aliens / boss / UFO
spawn enemy shots
move bullets and resolve collisions
update power-ups
advance level if needed
```

SDL3 calls the same stepping path from its own event/render loop. It
captures previous positions before a tick and interpolates during draw.
Rendering should not change gameplay state.

## Replays

Replays store inputs, not snapshots or video.

```cpp
struct InputFrame {
    uint32_t tick;
    uint8_t p1;
    uint8_t p2;
};
```

The replay header stores seed, difficulty, mode, player, and optionally
expected score/level. The body stores input frames, with RLE compression
for repeated masks.

SDL3 records and plays replays. Terminal mode also verifies them:

```bash
./build/si_pro --verify-replay player_last.rpl
```

Be careful when changing simulation order, RNG calls, input masks, or
replay parsing. Old replays may stop verifying if those change.

## Events

`src/core/game_event.h` defines small gameplay events emitted by `Game`
each tick:

- `BulletFired`
- `AlienKilled`
- `PowerupCollected`
- `BossPhaseChanged`
- `PlayerHit`
- `LevelCleared`

SDL3 audio and Director AI can consume these events instead of guessing
from state changes. The event buffer is cleared at the start of each
simulation step.

## Director AI

The Director is separate from the player AI.

- `AISource` chooses player actions.
- `Director` watches the run and adjusts pressure.

The Director computes modifiers for alien shooting, alien movement, and
power-up drops. The SDL3 loop applies those modifiers to `Game`. Replay
playback disables Director modifiers so a replay is not changed by live
adaptive difficulty.

## Networking

LAN co-op uses TCP input lockstep. Each peer sends its local action mask
and receives the other player's mask each tick. Both sides start from the
same seed and difficulty.

The code path is intentionally small:

```text
KeyboardSource -> CoopSource -> Game
```

The current networking path is terminal-first. SDL3 co-op would need UI
work before it is pleasant to use.

## Headless Tools

`Game::run_headless()` runs simulation without rendering or sleeping.
It is used by:

- replay verification
- benchmark mode
- AI training
- GA tuning
- determinism tests

This is one of the cleaner parts of the project. Keep it boring.

## Persistent Files

Most runtime files are plain text.

| File | Purpose |
|---|---|
| `<user>_save.dat` | Saved run state |
| `<user>_record.dat` | Personal best |
| `<user>_stats.dat` | Lifetime stats |
| `<user>_ach.dat` | Achievements |
| `<user>_curves.csv` | Per-level telemetry |
| `<user>_last.rpl` | Latest human replay |
| `<user>_ai_last.rpl` | Latest AI demo replay |
| `leaderboard.dat` | Top scores |
| `*.rpl` | Replay files |
| `*.lvl` | Level files |
| `ai_train.csv` | AI training output |
| `ai_evolve.csv` | GA tuning output |
| `si_pro.cfg` | Config |
| `si_pro.log.<pid>` | Logs |

Plain text makes these files easy to inspect and diff. The tradeoff is
that parsers need to stay tolerant of older files.

## Replay Format

Example:

```text
HEADER seed=12345 diff=2 mode=solo player=anurag score=4250 level=3
0 4 0
1 4 0
RUN 47 0 0 2
49 1 0
```

Frame lines use:

```text
<tick> <p1_mask> <p2_mask>
```

RLE lines use:

```text
RUN <count> <p1_mask> <p2_mask> <start_tick>
```

The loader accepts both formats.

## Level Format

Example:

```text
NAME My Test Wave
SEED 42 AUTHOR anurag
TIMING 6 22
BOSS 1
X.X.X.X.X.X
.X.X.X.X.X.
X.X.X.X.X.X
####
####
```

Alien rows use `X` for present and `.` for empty. Shield rows use `#`
for a brick and `.` for empty.

## Tests Worth Knowing

| Test | Why it matters |
|---|---|
| `test_determinism.cpp` | Same seed/input should produce same snapshot |
| `test_replay.cpp` | Replay files round-trip |
| `test_events.cpp` | Event emission stays visible |
| `test_director.cpp` | Director beat behavior |
| `test_config.cpp` | Config keys stay persistent |

Run everything with:

```bash
ctest --test-dir build --output-on-failure
```

## What To Read First

If you are new to the codebase, read in this order:

1. `src/game/game.h`
2. `src/input/input_source.h`
3. `src/game/game.cpp`
4. `src/game/game_step.cpp`
5. `src/main_sdl3.cpp`
6. `src/persistence/replay_file.cpp`
7. `tests/test_determinism.cpp`

That gives you the shape of the project without reading every file.
