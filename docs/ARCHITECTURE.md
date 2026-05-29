# Architecture

This document explains the structure of `si_pro` for someone (e.g. a
viva examiner, or future-me) who wants to navigate the code without
reading every line. Read it alongside the top-level `README.md`.

## One-paragraph summary

`si_pro` is a single binary built from ~4700 lines of modern C++17,
split across ~70 source files in 13 directories. The game state lives
in a single `Game` class. Game state is driven each tick by an abstract
`IInputSource` — one polymorphic interface satisfied by five concrete
classes (keyboard, AI, replay, network co-op, AI-vs-AI). Persistence is
six independent plain-text formats. Networking is input-lockstep over
TCP with seeded RNG synchronization. Headless analysis tools
(benchmark, replay verifier, training, GA evolution) reuse the same
`Game::run_headless` entry point that tests use.

## Dependency graph

The build graph is strictly one-way. There are no cyclic includes;
every file in a lower layer compiles without seeing anything from a
higher layer.

```
                            main.cpp
                                │
                ┌───────────────┴───────────────┐
                ▼                               ▼
              ui/             tools (benchmark, evolve, verify)
                │                               │
        ┌───────┴───────┐                       │
        ▼               ▼                       │
     editor/         menus/                     │
        │               │                       │
        └───────┬───────┴───────────────────────┘
                ▼
              game/  (orchestrator)
                │
        ┌───────┴───────┐
        ▼               ▼
      input/         persistence/
        │               │
        └───────┬───────┘
                ▼
              core/  (entities, RNG, constants, action masks)
                │
        ┌───────┴────────┐
        ▼                ▼
     render/          platform/  (POSIX vs Win32 abstraction)
                          │
                       net/      (TCP socket wrapper)

    config/ and debug/ (logger) are leaf modules used everywhere.
    i18n/ is used by ui/ only.
```

## Directory map

| Directory             | Role                                                         |
|-----------------------|--------------------------------------------------------------|
| `src/platform/`       | OS abstraction. POSIX termios + BSD sockets vs Win32 conio + Winsock. |
| `src/core/`           | Constants, ANSI colors, RNG, entity structs, difficulty, version. |
| `src/config/`         | `key=value` config file loader; CLI flag parser.             |
| `src/debug/`          | Thread-safe leveled logger.                                  |
| `src/render/`         | Flicker-free terminal render buffer (`RBuf`).                |
| `src/persistence/`    | Six plain-text file formats: save, leaderboard, stats, achievements, replay, level, telemetry. |
| `src/net/`            | RAII `TCPSocket` + `net_host` / `net_join` helpers.          |
| `src/input/`          | `IInputSource` abstraction and five implementations.         |
| `src/game/`           | The `Game` class, split across three .cpp files for navigability. |
| `src/editor/`         | In-terminal level editor.                                    |
| `src/ui/`             | Banner, menus, mode runners, tools (verify/evolve/benchmark/ai-vs-ai). |
| `src/i18n/`           | English + Hindi UI strings.                                  |
| `tests/`              | Five unit-test suites with a tiny dependency-free framework. |

## Key abstractions

### `IInputSource`

The single polymorphic interface that lets one `Game::run` loop drive
every mode. Defined in `src/input/input_source.h`:

```cpp
struct IInputSource {
    virtual std::uint8_t poll(std::uint32_t tick,
                              const Game& g,
                              int playerId) = 0;
};
```

Implementations:

| Class            | Used by                                                |
|------------------|--------------------------------------------------------|
| `KeyboardSource` | Human play. Reads from the atomic flags set by the input thread. |
| `AISource`       | AI demo, AI vs AI, training, GA evolution.             |
| `ReplaySource`   | `[6] Replay`, `--verify-replay`, determinism tests.    |
| `CoopSource`     | Network co-op. Dispatches on `playerId == self_player_` to either send our keyboard mask or receive the peer's. |
| `Dual` (anon)    | AI vs AI. Wraps two `AISource` instances, one per player slot. |

Adding a new control source — say, a recorded-gesture replayer or a
network spectator — is one new class implementing `poll()`. The Game
loop never needs to know.

### `Game` class

Defined in `src/game/game.h`, split across:
- `game.cpp` — constructors, `run`, `run_headless`, `apply_action`,
  `step`, `snap`, debug console.
- `game_step.cpp` — per-tick game logic: alien movement, shooting,
  collisions, level transitions, boss AI.
- `game_render.cpp` — drawing the world to `RBuf` and the HUD line.

The split exists to keep each file under ~350 lines and navigable.
They share the same private state through `game.h`.

### Action masks

Defined in `src/core/action.h`:

```cpp
LEFT    = 1 << 0
RIGHT   = 1 << 1
SHOOT   = 1 << 2
PAUSE   = 1 << 3
QUIT    = 1 << 4
CONSOLE = 1 << 5
```

This is the unit of currency between input sources, the game loop, the
network protocol, and the replay format. One byte per player per tick.

### `InputFrame`

```cpp
struct InputFrame { uint32_t tick; uint8_t p1, p2; };
```

This struct is the single primitive that powers both the replay
format and the network protocol. A replay file is a sequence of
`InputFrame`s (RLE-encoded). A network tick is one byte from each peer.

## Networking protocol

Input-lockstep over TCP. Wire format:

```
                    HOST                              CLIENT
                      │                                 │
        Listen(7777)  │                                 │
        Accept(client)│ <───── connect(host_ip) ───────│
                      │                                 │
        sendLine      │ ─── "HELLO <seed> <diff>\n" ──> │
                      │                                 │
                      │ <───────────── "OK\n" ────────  │ sendLine
                      │                                 │
                      │                                 │
   per tick (12.5 fps): every peer sends 1 byte (its own player's input
   mask) and receives 1 byte (the peer's). Both peers seed std::mt19937
   from the same value and step their Game instance identically. No
   state-sync packets are ever sent.
```

**Why this works.** The simulation is fully deterministic — same seed +
same input sequence ⇒ same result, byte for byte. As long as both
peers agree on the seed (set by HELLO) and on each tick's inputs
(exchanged each frame), they cannot drift. Desync is structurally
impossible.

**Why TCP, not UDP.** A 12.5-fps real-time loop over loopback or LAN
has trivial latency. TCP gives us ordering and reliability for free.
UDP would force us to write our own ACK/retry. Not worth it.

**Failure mode.** If either peer disconnects, the next `recvAll(&m, 1)`
returns `false`. `CoopSource` sets a shared `dead` atomic flag, which
`Game::run` checks each frame and treats as game-over with a
"** PEER DISCONNECTED **" banner.

## Determinism guarantees

Two facts the test suite verifies:

1. `std::mt19937` produces the same sequence for the same seed — test
   `test_rng` checks 1000 samples and reseed behavior.
2. Two `Game` instances constructed with the same seed and difficulty
   produce identical `snap()` snapshots — test `test_determinism`.

These two facts together imply that the network co-op cannot desync if
the underlying TCP stream is intact. (It cannot lose or reorder bytes,
because TCP.)

## Concurrency

Three things run concurrently in solo / co-op mode:

1. **Main thread** — drives `Game::run`. Polls input sources, calls
   `step`, renders, sleeps 80 ms.
2. **Input thread** — `input_thread_main` in
   `src/input/input_source.cpp`. Blocks in `kb_available()`,
   sets atomic flags on `InputState`. The main thread reads those
   flags via `KeyboardSource::poll()`.
3. **Logger** — protected by `std::mutex`; safe to call from any
   thread.

There are no other threads. The network co-op does its socket I/O on
the main thread, inside `CoopSource::poll()`. This is fine because
TCP `recv`/`send` calls at ~12 Hz over loopback or LAN are nowhere
near saturating a thread.

## Headless game loop

`Game::run_headless(p1, p2, max_ticks)` simulates without rendering
or sleeping. It's the entry point shared by:

- `test_determinism.cpp` — invariant checks
- `tools::verify_replay` — `--verify-replay`
- `tools::benchmark` — `--benchmark N`
- `tools::evolve_ai` — `--evolve-ai N`
- `run_train_ai` — `--train-ai N`

The split between `run` (interactive: render + sleep + cursor control)
and `run_headless` (pure simulation) is the cleanest seam in the
codebase. Almost everything analyzable was unlockable by making this
one cut.

## File formats

All persistent data is plain text. Easy to grade. Easy to diff.

| File                    | Format                                                  |
|-------------------------|---------------------------------------------------------|
| `<user>_save.dat`       | One whitespace-separated line of game state + alien grid. |
| `<user>_record.dat`     | `<score> <level>\n<difficulty name>\n`                  |
| `<user>_stats.dat`      | Ten ints on one line.                                   |
| `<user>_ach.dat`        | `<key> <unlocked?>\n` per line.                         |
| `<user>_curves.csv`     | Per-level telemetry CSV.                                |
| `<user>_last.rpl`       | Auto-saved replay of the last solo run.                 |
| `leaderboard.dat`       | Top-10 records.                                         |
| `*.rpl`                 | Replay file, header + RLE-encoded body.                 |
| `*.lvl`                 | Level file: name, seed, alien grid, shield grid.        |
| `ai_train.csv`          | Training output: one row per game.                      |
| `ai_evolve.csv`         | GA output: best/mean + winning weights per generation.  |
| `si_pro.cfg`            | `key = value` config; written by the in-game Settings.  |
| `si_pro.log.<pid>`      | Timestamped log lines (when logging enabled).           |

### Replay format details

```
HEADER seed=12345 diff=2 mode=solo player=anurag score=4250 level=3
0 4 0
1 4 0
RUN 47 0 0 2
49 1 0
...
```

The header carries the seed, difficulty, mode, player name, and
optionally the expected final score and level (used by
`--verify-replay`).

The body is one line per tick of the form `<tick> <p1_mask> <p2_mask>`,
or one RLE compression line of the form
`RUN <count> <p1_mask> <p2_mask> <start_tick>` representing `count`
consecutive ticks with the same masks starting at `start_tick`. RLE
runs are emitted when 3 or more consecutive ticks have identical
masks (common: most ticks have no input). The loader auto-detects the
line type, so older non-RLE replays still load.

### Level format details

```
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

Three lines of header (`NAME`, `SEED..AUTHOR`, `TIMING`, `BOSS`),
three lines of 11-column alien grid (`X` = present, `.` = empty),
two lines of 4-column shield template (`#` = brick, `.` = empty).

## AI utility function

Defined in `src/input/ai_source.cpp`. Each tick, three candidate
x-positions are scored:

```
U(x) = − w_danger · danger(x)
       + w_align  · alignment(x)
       + w_pickup · pickup(x)
       − w_center · |x − W/2|
```

- `danger(x)` — sum over incoming alien bullets, weighted by inverse
  vertical distance. Bullets close to the player and in the column
  count more.
- `alignment(x)` — proximity to the closest live target (alien, UFO,
  or boss).
- `pickup(x)` — proximity to falling power-ups.
- The center bias keeps the AI from getting stuck in a corner without
  escape routes.

The action with the highest `U(x)` is chosen. Shooting is decoupled:
fire when a target is in the column AND no friendly bullet already
occupies the lane.

Three preset profiles ship in the box (`aggressive`, `defensive`,
`balanced`). The `--evolve-ai` GA mutates these weights and selects
for fitness across many simulated games.

## Settings persistence flow

1. `main()` calls `load_config("si_pro.cfg", cfg)` at startup.
2. Settings flow into `ui::opts()` (rendering) and `i18n::set_language()`
   (UI text).
3. The user enters the menu, presses `S`, edits values, presses `W`.
4. `save_config("si_pro.cfg", cfg)` writes the new values back.
5. Changes take effect immediately for `ui::opts()` and language;
   the rest applies on next launch or next `run_solo()` call.

## Why this is structured the way it is

A few specific architectural decisions I want to justify:

- **Single-file `Game` class.** Boss / UFO / power-ups all touch the
  same set of vectors; splitting them across classes would require
  exposing all state through public accessors, which is worse than the
  current arrangement. The Game *file* is split (three .cpp); the
  class isn't.

- **`IInputSource` polymorphism.** This is the dependency inversion
  that lets us add new modes (AI vs AI, replay, networking) without
  touching the run loop. Five implementations live behind it; the run
  loop knows about exactly one.

- **Plain-text formats everywhere.** A grader can open a `.rpl` and
  see what happened. A `.lvl` is hand-editable. A `.dat` is `cat`-able.
  This matters for academic work; a binary format would have been
  marginally faster and infinitely worse to grade.

- **Headless simulation as a separate entry point.** I tried at one
  point to make `Game::run` itself skip rendering when a flag was set.
  It worked but the conditionals leaked into every render call.
  Splitting it into two functions sharing a `step()` helper was much
  cleaner.

- **No external dependencies.** Just the C++ standard library and OS
  sockets. This matters for cross-platform builds and for graders who
  might not have package managers configured.

## What to read first

If you have one hour to understand the codebase:

1. `src/game/game.h` — the central data model.
2. `src/input/input_source.h` — the polymorphism that makes
   everything else work.
3. `src/game/game.cpp` — `Game::run` and `Game::step`.
4. `src/input/ai_source.cpp` — the AI utility function in 30 lines.
5. `src/input/coop_source.cpp` — how the network protocol is just
   send-then-recv per tick.
6. `tests/test_determinism.cpp` — the proof that lockstep can't desync.

That's about 600 lines total and covers 80% of the system.
