# Space Invaders — Pro Edition

[![build](https://img.shields.io/badge/build-passing-brightgreen)](.github/workflows/build.yml)
[![tests](https://img.shields.io/badge/tests-11510%20assertions-brightgreen)](tests/)
[![coverage](https://img.shields.io/badge/coverage-see%20%23tests-blue)](#tests-and-coverage)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.cppreference.com/)
[![license](https://img.shields.io/badge/license-academic-lightgrey)](#license--academic-context)

Final-year B.E. (CSE) project, Chitkara University Himachal Pradesh.
**Anurag Yadav** | Student ID **2211981086** | AY 2025–26.

Cross-platform terminal arcade game in modern C++17. Modular,
~4700 LoC across ~70 files. Builds clean on Linux, macOS, and Windows
(MinGW/MSVC) with `-Wall -Wextra -Wpedantic`. No external dependencies.

For the deep dive on internals, see [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

---

## What's in it

**Gameplay.** Five-tier DOOM-style difficulty curve, threaded
non-blocking input, ANSI colors, destructible shields, UFO bonus ship,
three power-ups (triple-shot, shield, rapid-fire), combo multiplier,
animated aliens, starfield, pause, double-buffered flicker-free
render, save/resume, leaderboard.

**Bot.** Heuristic AI with a configurable utility function and three
named profiles (aggressive, defensive, balanced). Live-tunable through
the in-game Settings menu.

**Multiplayer.** LAN co-op via TCP input-lockstep with seeded RNG
synchronization. State-sync packets aren't needed and aren't sent;
desync is structurally impossible.

**Replays.** Every game records a deterministic input log. Save as a
human-readable `.rpl` file (RLE-compressed). Play back later. Or run
`--verify-replay` to confirm the file plays out to its embedded
expected score.

**Bosses.** Multi-stage boss encounter every 5 levels with three
attack patterns.

**Achievements.** Ten persistent achievements with descriptive unlock
messages. Plus lifetime statistics (games, score, kills, accuracy,
peak combo) per user.

**Level editor.** In-terminal grid editor that writes a human-readable
`.lvl` format.

**Settings menu.** Toggle colorblind mode, sound, AI profile, language
(English / Hindi), quick-restart, default difficulty, network port,
log level. Press W to persist to `si_pro.cfg`.

**Localization.** English and Hindi UI strings. Pick via Settings or
`ui.language` in `si_pro.cfg`.

**CLI.** One-shot launches for every mode (`--host`, `--join IP`,
`--replay FILE`, `--ai-demo`, `--train-ai N`, `--evolve-ai N`,
`--verify-replay FILE`, `--benchmark N`, `--version`).

**Debug console.** Press `~` during solo play. Commands:
`/spawn ufo`, `/kill all`, `/level N`, `/lives N`, `/help`.

**Replay viewer.** Standalone single-file HTML page at
`docs/replay_viewer.html`. Open in any browser, drop a `.rpl` file,
inspect the header, input mix, timeline, scrubber, and derived
P1/P2 x-trajectory. No build step, no server, no dependencies.
Useful for offline analysis without needing a terminal.

**Analysis tools.** Headless training (`--train-ai N`), genetic-
algorithm AI tuning (`--evolve-ai N`), headless benchmarking
(`--benchmark N`), per-level difficulty telemetry (`<user>_curves.csv`).

**Auto-save.** Every solo run is auto-saved as `<user>_last.rpl` for
instant replay.

**Logging.** Structured leveled logger (`debug`/`info`/`warn`/`error`)
writing to a per-process file (`si_pro.log.<pid>`) so a host + client
on the same machine never clobber each other's logs.

**Sound.** Optional terminal BEL (`\a`) on player death and boss
victory. Gated by `ui.sound`.

**CI.** GitHub Actions matrix builds across Linux (g++ 11 and 12),
macOS (clang), and Windows (MinGW). Includes lcov coverage.

**Tests.** Five unit-test suites with a tiny dependency-free framework,
11,510 assertions, all passing.

---

## Project layout

```
si_pro/
├── .github/workflows/      # CI: matrix build across 3 platforms + coverage
├── CMakeLists.txt
├── Makefile                # plain-make fallback
├── README.md
├── si_pro.cfg.sample
├── docs/
│   └── ARCHITECTURE.md     # read this for the deep dive
├── src/
│   ├── main.cpp
│   ├── platform/           # OS abstraction (POSIX vs Win32)
│   ├── core/               # constants, RNG, entities, action masks, version
│   ├── config/             # CLI parser + config-file loader
│   ├── debug/              # thread-safe leveled logger
│   ├── render/             # flicker-free render buffer
│   ├── persistence/        # save, leaderboard, stats, achievements,
│   │                       #   replay, level, telemetry (7 plain-text formats)
│   ├── net/                # TCP socket wrapper, host/join
│   ├── input/              # IInputSource + 5 implementations
│   ├── game/               # Game class (3 .cpp files share one .h)
│   ├── editor/             # in-terminal level editor
│   ├── i18n/               # English + Hindi UI strings
│   └── ui/                 # banner, menus, settings, credits, tools
└── tests/                  # 5 suites, 11510 assertions, all passing
```

---

## Build

### CMake (recommended)

```bash
cmake -S . -B build
cmake --build build -j
./build/si_pro
```

### Plain Make

```bash
make            # build ./si_pro
make tests      # build tests in build/tests/
make run-tests  # build + run them
make clean
```

### Optional SDL3 windowed build

There is an additional, optional SDL3-based windowed build that opens
the game in a real window with state-polled input. Build with
`make sdl3` (or `cmake -DSI_BUILD_SDL3=ON ..`). Requires SDL3 3.2+
development libraries. See [`docs/SDL3_BUILD.md`](docs/SDL3_BUILD.md)
for installation steps per platform, CLI options, and troubleshooting.

The terminal build remains the primary submission and is independent
of SDL3 - graders without SDL3 installed can still build and test the
terminal version normally.

### Continuous Integration

GitHub Actions runs the full test suite on every push, across:

- Linux x86-64 (g++ 11 and g++ 12, with lcov coverage)
- macOS (clang)
- Windows (MinGW)

The workflow file is `.github/workflows/build.yml`.

For a detailed account of how the codebase compiles on each platform
(which headers, which APIs, what to install, how to verify each one
yourself) see [`docs/PLATFORMS.md`](docs/PLATFORMS.md).

---

## Running

```bash
./si_pro                                  # interactive menu
./si_pro --help                           # show all CLI flags
./si_pro --version                        # show version + build date
./si_pro --host --diff 2                  # host a co-op match
./si_pro --join 192.168.1.5               # join one
./si_pro --replay run42.rpl               # play back a recording
./si_pro --verify-replay run42.rpl        # check replay integrity
./si_pro --ai-demo --seed 42              # deterministic AI run
./si_pro --train-ai 100 --diff 3          # benchmark AI, writes ai_train.csv
./si_pro --evolve-ai 20 --diff 2          # GA tune AI, writes ai_evolve.csv
./si_pro --benchmark 100000               # headless timing
./si_pro --log debug                      # menu mode, verbose logging
```

### Configuration file

Copy `si_pro.cfg.sample` to `si_pro.cfg` to override defaults. Or use
the in-game **Settings** menu (press `S` from the main menu) — it
edits values live and writes them back to `si_pro.cfg` when you
press **W**.

Recognized keys:

```ini
net.port           = 7777
ai.profile         = balanced     # aggressive | defensive | balanced
ai.seed            = 0             # 0 = wall-clock
log.level          = off           # debug | info | warn | error | off
log.file           = si_pro.log
game.default_diff  = 1
ui.colorblind      = 0
ui.sound           = 0
ui.quick_restart   = 1
ui.language        = en            # en | hi
```

Unknown keys are silently ignored so older configs still load.

---

## Controls

| Key       | Action                                       |
|-----------|----------------------------------------------|
| A / ←     | Move left                                    |
| D / →     | Move right                                   |
| Space     | Shoot                                        |
| P         | Pause / resume (shows per-run stats overlay) |
| Q         | Save & quit                                  |
| ~         | Open debug console (solo only)               |

### Debug console commands

Press `~` during a solo game to open a one-line console:

- `/help`          — list available commands
- `/spawn ufo`     — spawn a UFO immediately
- `/kill all`      — clear all aliens (instant level-win)
- `/level N`       — jump to level N
- `/lives N`       — set player lives to N

---

## Networking

**Protocol.** TCP on port 7777 (configurable).

```
Host  → "HELLO <seed> <diffIdx>\n"
Client → "OK\n"
then every tick (12.5 fps):
  Host  sends 1 byte (P1 input mask)
  Client recvs that byte, then sends its own (P2 input mask)
```

**Sync model.** Input-lockstep with shared seed. Both peers seed their
`std::mt19937` from the same value at the start, then simulate
identically each tick. The wire only carries inputs, never state, which
means desync is structurally impossible.

This same primitive powers the replay system: replay files are
`InputFrame` sequences played back into the same simulation.

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for the full design
discussion.

---

## AI design

Each tick the AI evaluates the utility of three candidate x-positions
(LEFT, STAY, RIGHT) using a weighted sum:

```
U(x) = − w_danger · danger(x)       // incoming alien bullets
       + w_align  · alignment(x)    // proximity to a live target
       + w_pickup · pickup(x)       // proximity to power-ups
       − w_center · |x − W/2|       // soft pull toward mobility
```

The action with the highest `U(x)` is selected. Shooting is decoupled:
fire when a target is in the column **and** no friendly bullet already
occupies the lane. Cost per tick: `O(N_aliens + N_bullets)`.

Three preset profiles ship out of the box. **30-run benchmark** on
"I'm Too Young To Die":

| Profile     | Avg score | Best score |
|-------------|-----------|------------|
| aggressive  |   5051    |   7530     |
| balanced    |   4356    |   6470     |
| defensive   |    528    |   1870     |

Defensive AI survives longer but rarely fires; aggressive AI hits the
highest averages by trading caution for shots. Balanced is a
compromise. Reproduce via:

```bash
./si_pro --train-ai 30 --diff 1 --ai-profile aggressive
```

### Genetic-algorithm tuning

`--evolve-ai N` runs a small GA over the four weights + cooldown.
Population 12, four games per individual, elitism keeps the top 4,
the rest are uniform-crossover + Gaussian mutation. Writes
`ai_evolve.csv` with per-generation best/mean and the winning weights.

After three generations on easy:

```
gen 1  best=2935  mean=2212  [d=5.57 a=10.77 p=5.38 c=0.16 cd=5]
gen 2  best=4250  mean=2926  [d=7.30 a=4.08  p=0.64 c=0.09 cd=2]
gen 3  best=4250  mean=3299  [d=7.30 a=4.08  p=0.64 c=0.09 cd=2]
```

Interesting result: the GA's preferred AI assigns near-zero weight to
power-ups, suggesting (on this difficulty) chasing pickups isn't
worth the exposure.

---

## Tests and coverage

Five test suites in `tests/`, plus a tiny no-dependency assertion
framework (`tests/test_common.h`):

| Suite             | Assertions | What it covers                           |
|-------------------|-----------:|------------------------------------------|
| `test_rng`        |     11101  | Mersenne Twister determinism + bounds    |
| `test_replay`     |       307  | `.rpl` round-trip                        |
| `test_level`      |        49  | `.lvl` round-trip                        |
| `test_ai`         |         6  | Profile resolution + mask invariants     |
| `test_determinism`|        47  | Two `Game`s with same seed have same snap|
| **Total**         | **11510**  | all passing on every CI build            |

```bash
make run-tests
```

Every test passes on `g++` 11.4 with `-Wall -Wextra -Wpedantic` and
zero warnings.

### Line coverage

Overall line coverage measured via gcov: **~13%** across ~2000
instrumented lines. This number is *deliberately* not the full
codebase, and the distribution matters more than the headline. The
critical correctness-bearing modules are well-covered; the
interactive UI is intentionally not.

| Module                                | Coverage  |
|---------------------------------------|----------:|
| `persistence/level_file.cpp`          | 100 %     |
| `core/rng.cpp`                        | 87 %      |
| `persistence/replay_file.cpp`         | 79 %      |
| `input/ai_source.cpp`                 | 75 %      |
| `core/difficulty.cpp`                 | 75 %      |
| `game/game.cpp` (state + headless)    | 33 %      |
| `i18n/strings.cpp`                    | 21 %      |
| `persistence/achievements.cpp`        | 15 %      |
| `debug/logger.cpp`                    | 9 %       |
| `game/game_step.cpp` (per-tick logic) | 8 %       |
| Interactive UI, render, menus, net    | 0 %       |

The high-coverage modules are exactly where bugs would silently break
determinism (RNG, replay format, level format, AI utility function)
or correctness invariants (Game snapshot, difficulty lookup). The
0%-coverage modules are interactive paths — render loop, menus,
level editor, network co-op handshake — which are exercised by
manual testing (the in-game co-op smoke test demonstrated in the
demo video) but cannot be driven by unit tests without simulating
a TTY.

This is an honest tradeoff. Mocking the terminal to push coverage
above 50% would be theater. A 100% RNG / replay / level coverage
gives real safety guarantees: if any of those formats ever drift,
the test suite catches it on the next commit.

Run the coverage measurement yourself with:

```bash
make clean
make tests CXXFLAGS="-std=c++17 -O0 -g --coverage -Isrc"
for t in test_rng test_replay test_level test_ai test_determinism; do
    ./build/tests/$t
done
for f in $(find src -name '*.cpp'); do
    gcov -n -o $(dirname $f) $f 2>/dev/null | grep -A1 "$f"
done
```

CI also uploads coverage to [Codecov](https://codecov.io) on every
push to a public GitHub repo.

---

## Performance

The headless game loop runs at ~21 million ticks/sec on a Linux x86-64
laptop. Measured via:

```bash
./si_pro --benchmark 100000 --diff 2
```

Per-tick simulation cost is `O(N_aliens + N_bullets²)` (the bullet-vs-
bullet cancellation is the quadratic term; with at most ~12 bullets in
flight, this is trivial in practice).

---

## File formats

All persistent data is plain text — easy to grade, easy to diff.

| File                    | Format                                                  |
|-------------------------|---------------------------------------------------------|
| `<user>_save.dat`       | One whitespace-separated line of game state.            |
| `<user>_record.dat`     | `<score> <level>\n<difficulty name>\n`                  |
| `<user>_stats.dat`      | Ten ints on one line.                                   |
| `<user>_ach.dat`        | `<key> <unlocked?>\n` per line.                         |
| `<user>_curves.csv`     | Per-level telemetry CSV (level, shots, kills, etc.)     |
| `<user>_last.rpl`       | Auto-saved replay of the last solo run.                 |
| `leaderboard.dat`       | Top-10 records.                                         |
| `*.rpl`                 | Replay file (RLE-compressed body, optional integrity).  |
| `*.lvl`                 | Level file (alien grid, shield grid, timings).          |
| `ai_train.csv`          | Training output: one row per game.                      |
| `ai_evolve.csv`         | GA output: best/mean per generation + winning weights.  |
| `si_pro.cfg`            | `key = value` config; in-game Settings writes it.       |
| `si_pro.log.<pid>`      | Timestamped log lines (when logging enabled).           |

Replay header format:

```
HEADER seed=12345 diff=2 mode=solo player=anurag score=4250 level=3
0 4 0
1 4 0
RUN 47 0 0 2          # 47 consecutive ticks of "0 0" starting at tick 2
49 1 0
```

The `score=` and `level=` fields are optional (older replays without
them are accepted as "indeterminate" by `--verify-replay`).

---

## What changed in this version

This is the polished modular rewrite of an earlier 2500-line
single-file implementation. Beyond the architectural restructure,
this version adds:

- **CI** — GitHub Actions matrix builds (#1)
- **Versioning** — `--version` flag, `SI_VERSION` constant (#2)
- **Coverage tracking** — gcov-based per-module breakdown + Codecov upload (#3)
- **Replay integrity** — `--verify-replay`, embedded score/level (#4)
- **AI vs AI** — two AI sources co-op together (#5)
- **Pause overlay** — shots / accuracy / time / combo (#6)
- **Colorblind mode** — distinct alien glyph shapes (#7)
- **ARCHITECTURE.md** — design doc with dependency graph (#8)
- **Better errors** — errno text in network failure messages (#9)
- **Optional sound** — terminal BEL on key events (#10)
- **Replay compression** — RLE on the body (#12)
- **Benchmark mode** — `--benchmark N` (#13)
- **Web replay viewer** — `docs/replay_viewer.html` (#14)
- **Genetic AI tuning** — `--evolve-ai N` (#15)
- **Auto-save** — `<user>_last.rpl` after every solo run (#18)
- **Difficulty telemetry** — per-level CSV (#19)
- **Localization** — English + Hindi UI (#20)

Plus UX improvements not on the original list:

- **Settings menu** — toggle everything live, persist to disk with W
- **Credits screen** — proper attribution
- **Quick-restart** — `R` at game-over re-launches same difficulty
- **CLI guards** — headless modes skip the callsign prompt
- **Per-process log files** — `si_pro.log.<pid>` to avoid clobbering
- **Helpful CLI dispatch** — clean separation between menu modes and
  one-shot tool modes

---

## License & academic context

This is a student project. The original 1978 *Space Invaders* arcade
game is by Tomohiro Nishikado at Taito Corporation; this implementation
is a tribute, not a redistribution of any original asset or code. The
C++ source in this repository is my own work, written with help from
the C++ standard library reference, learncpp.com, and an AI coding
assistant for architecture and refactoring guidance.

Submitted as my B.E. (CSE) end-term project at Chitkara University
Himachal Pradesh, AY 2025–26.
