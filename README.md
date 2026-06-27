# Space Invaders - Pro Edition

My C++17 Space Invaders project. It started as my final-year college project,
and I kept working on it because it became a useful place to build real game
systems: windowed rendering, deterministic replays, bosses, Director AI, co-op,
custom levels, and tests.

The windowed build is the one to play. It is built on SDL3. The terminal build
is still here for tools, replay checks, benchmarks, and older development
workflows.

## What Is In The Game

- Windowed build with menus, audio, particles, stats, achievements, replay
  recording/playback, LAN co-op setup, custom level loading, and a level editor.
- Terminal build for headless checks and developer tools.
- Deterministic simulation driven by small input masks.
- Human-readable `.rpl` replay files.
- Director AI that can push or ease pressure during a run.
- Boss waves, UFO target, shields, combo scoring, and powerups.
- CMake build and CTest coverage for the important systems.

## Build

Use CMake. Platform notes are in [docs/PLATFORMS.md](docs/PLATFORMS.md).

### Windowed Build

Install the required windowing development libraries first.

```bash
cmake -S . -B build -DSI_BUILD_SDL3=ON
cmake --build build -j
./build/si_pro_sdl3
```

Useful run options:

```bash
./build/si_pro_sdl3 --ai-demo
./build/si_pro_sdl3 --diff 3
./build/si_pro_sdl3 --diff 1 --seed 12345 --user pilot
./build/si_pro_sdl3 --fullscreen
./build/si_pro_sdl3 --help
```

### Terminal / Tools Build

```bash
cmake -S . -B build
cmake --build build -j
./build/si_pro
```

Useful tool commands:

```bash
./build/si_pro --verify-replay player_last.rpl
./build/si_pro --benchmark 10000
./build/si_pro --legacy-menu
```

The Makefile is only a small wrapper around CMake:

```bash
make
make run-tests
make sdl3
```

## Common Build Problems

### CMake Tries To Use NMake On Windows

If you see this:

```text
Running 'nmake' '-?' failed with: no such file or directory
CMAKE_CXX_COMPILER not set
```

CMake picked the Visual Studio/NMake generator, but NMake is not installed or
not available in your terminal. If you are using MinGW, delete the old build
folder and configure with the MinGW generator:

```powershell
Remove-Item -Recurse -Force build
cmake -S . -B build -G "MinGW Makefiles" -DSI_BUILD_SDL3=ON
cmake --build build
```

Check that MinGW is visible first:

```powershell
where.exe g++
where.exe mingw32-make
g++ --version
mingw32-make --version
```

For MSYS2, the PATH entry is usually one of:

```text
C:\msys64\ucrt64\bin
C:\msys64\mingw64\bin
```

### CMake Says The Source Directory Has No CMakeLists.txt

This usually happens when you run the configure command from inside `build/`:

```powershell
cd build
cmake -S . -B build
```

Run CMake from the project root instead:

```powershell
cd D:\Project~\si_pro-finalCut
cmake -S . -B build -G "MinGW Makefiles" -DSI_BUILD_SDL3=ON
```

## Test

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The tests cover RNG behavior, replay files, custom level files, AI input,
Director behavior, gameplay events, config persistence, and simulation
snapshots.

## Controls

| Key | Action |
|---|---|
| A / Left Arrow | Move left |
| D / Right Arrow | Move right |
| Space | Shoot |
| P | Pause / resume |
| Q | Quit current run |
| F11 | Toggle fullscreen in the windowed build |
| M | Toggle mute in the windowed build |
| R | Restart from game-over |
| `~` | Open terminal debug console in solo mode |

## Replays

The windowed build writes replay files to the directory you launched the game from:

```text
<user>_last.rpl
<user>_ai_last.rpl
```

Open **Watch Replay** in the windowed build to browse and play them back. The
terminal build can also verify a replay without opening the game window:

```bash
./build/si_pro --verify-replay run42.rpl
```

Replay files are input logs, not video recordings. If you change simulation
order, RNG usage, input masks, or replay parsing, run the replay and
determinism tests.

## Project Layout

```text
src/core/          constants, entities, RNG, actions, difficulty, events
src/game/          main simulation
src/input/         keyboard, windowed input, AI, replay, co-op input
src/render/        terminal render buffer and windowed renderer
src/audio/         synthesized audio
src/director/      adaptive Director AI
src/persistence/   saves, stats, achievements, leaderboard, replays, levels
src/ui/            terminal and windowed menu flows
src/net/           TCP lockstep helpers
src/platform/      POSIX/Win32 abstraction
tests/             C++ test executables
docs/              platform build notes only
website/           static project website
```

## Config

Copy `si_pro.cfg.sample` to `si_pro.cfg`, or use the in-game settings menus
where available.

```ini
net.port           = 7777
ai.profile         = balanced
ai.seed            = 0
log.level          = off
log.file           = si_pro.log
game.default_diff  = 1
ui.colorblind      = 0
ui.sound           = 0
ui.quick_restart   = 1
ui.language        = en
sdl3.user          = pilot
sdl3.fullscreen    = 0
sdl3.muted         = 0
sdl3.director      = 1
sdl3.reduced_motion = 0
```

Unknown config keys are ignored so older local files keep working.

## Contributing

Small, focused pull requests are easiest to review. Good areas to help with:
platform fixes, windowed packaging, replay tooling, tests, controller input,
accessibility, and clear bug reports.

Before changing gameplay simulation, run the tests and check replay behavior.
That part of the project depends on deterministic updates.

## Credits

The original *Space Invaders* arcade game is by Tomohiro Nishikado at Taito.
This project is a tribute and does not include original Taito code or assets.
