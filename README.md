# Space Invaders - Pro Edition

[![build](https://img.shields.io/badge/build-passing-brightgreen)](.github/workflows/build.yml)
[![tests](https://img.shields.io/badge/tests-11510%20assertions-brightgreen)](tests/)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.cppreference.com/)
[![license](https://img.shields.io/badge/license-MIT-green)](LICENSE)

Space Invaders - Pro Edition is my C++17 arcade game based on the
classic Space Invaders idea. I originally built it as my final-year
college project, then kept working on it because the systems around the
game were fun to improve: replays, AI, bosses, co-op, achievements, and
an SDL3 version with real rendering and audio.

Run the SDL3 build first if you want to play the game. The terminal
build is still useful for tools: replay verification, LAN co-op, level
editing, localization, benchmarking, and AI experiments.

## Highlights

- SDL3 windowed build with particles, interpolation, menus, audio,
  persistent stats, achievements, replay recording, and replay playback.
- Terminal build with developer tools and no external runtime
  dependency.
- Deterministic simulation driven by small input masks.
- Human-readable `.rpl` replay files.
- Director AI that creates visible pressure/relief beats during play.
- Boss waves, UFO target, destructible shields, combo scoring, and
  power-ups.
- CMake/Make builds, CI, and focused C++ tests.

## Build And Run

### SDL3 Build

Install SDL3 3.2+ development libraries first. Platform notes are in
[docs/SDL3_BUILD.md](docs/SDL3_BUILD.md).

```bash
cmake -S . -B build -DSI_BUILD_SDL3=ON
cmake --build build --target si_pro_sdl3 -j
./build/si_pro_sdl3
```

Useful options:

```bash
./build/si_pro_sdl3 --ai-demo
./build/si_pro_sdl3 --diff 3
./build/si_pro_sdl3 --diff 1 --seed 12345 --user pilot
./build/si_pro_sdl3 --fullscreen
./build/si_pro_sdl3 --help
```

### Terminal / Tools Build

The terminal build only needs a C++17 compiler.

```bash
cmake -S . -B build
cmake --build build -j
./build/si_pro
```

Plain Make is also supported:

```bash
make
make run-tests
```

## Controls

| Key | Action |
|---|---|
| A / Left Arrow | Move left |
| D / Right Arrow | Move right |
| Space | Shoot |
| P | Pause / resume |
| Q | Quit current run |
| F11 | Toggle fullscreen in SDL3 |
| M | Toggle mute in SDL3 |
| R | Restart from game-over screen |
| `~` | Open terminal debug console in solo mode |

Reduced Motion can be enabled from SDL3 Settings or by setting
`sdl3.reduced_motion = 1` in `si_pro.cfg`.

## Feature Split

| Feature | SDL3 | Terminal/tools |
|---|---:|---:|
| Windowed rendering, particles, shake | Yes | No |
| Synthesized audio and music | Yes | Terminal beep only |
| Menus, stats, achievements | Yes | Yes |
| Director Beats | Yes | Simulation/config support |
| Reduced Motion | Yes | Config only |
| AI demo | Yes | Yes |
| Replay recording | Yes | Yes |
| Replay playback | Yes | Yes |
| Replay verification | Terminal-first | Yes |
| LAN co-op | Planned | Yes |
| Level editor | Planned | Yes |
| Hindi localization | Terminal-first | Yes |
| Benchmark / AI training / GA tuning | Tools only | Yes |

## Replays

SDL3 writes the latest human run to:

```text
<user>_last.rpl
```

AI demo runs use:

```text
<user>_ai_last.rpl
```

The files are written to the directory you launched the game from. To
watch one in SDL3, choose **Watch Replay** and enter the filename.
Typing `alpha_last` is enough; SDL3 adds `.rpl` and also checks the
common repo/build/executable locations used during development.

The terminal build can also verify replay integrity:

```bash
./build/si_pro --replay run42.rpl
./build/si_pro --verify-replay run42.rpl
```

Replay files are input logs, not video recordings. If you change core
simulation order, RNG usage, action masks, or replay parsing, run the
replay and determinism tests.

## Project Structure

```text
src/core/          shared constants, entities, RNG, actions, difficulty
src/game/          main simulation
src/input/         keyboard, SDL3 keyboard, AI, replay, co-op input
src/render/        terminal render buffer and SDL3 renderer
src/audio/         SDL3 synthesized audio
src/director/      adaptive Director AI
src/persistence/   saves, stats, achievements, leaderboard, replays, levels
src/ui/            terminal and SDL3 menu flows
src/net/           TCP lockstep helpers
src/platform/      POSIX/Win32 abstraction
tests/             C++ test executables
docs/              build, architecture, platform, and release notes
website/           static project website
```

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the design notes.

## Tests

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The tests cover RNG behavior, replay files, level files, AI actions,
Director behavior, gameplay events, config persistence, and deterministic
simulation snapshots.

## Configuration

Copy `si_pro.cfg.sample` to `si_pro.cfg` or use the in-game settings
menus where available.

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
sdl3.user           = pilot
sdl3.fullscreen     = 0
sdl3.muted          = 0
sdl3.director       = 1
sdl3.reduced_motion = 0
```

Unknown keys are ignored so older config files keep working.

## Contributing

Contributions are welcome, especially small changes that make the game
easier to build, test, play, package, or understand. Start with
[CONTRIBUTING.md](CONTRIBUTING.md) and [ROADMAP.md](ROADMAP.md).

## Origins
The original 1978 *Space Invaders* arcade game is by Tomohiro Nishikado
at Taito Corporation. This project is a tribute and does not include
original Taito code or assets.

