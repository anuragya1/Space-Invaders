# SDL3 Build Quick Start

The terminal version of Space Invaders is the *primary* submission and
builds with `make` exactly as before. This document covers the additional
SDL3 windowed build.

## What the SDL3 build provides

A windowed (1120 x 512 pixels) version of the game with:

- Vsync-paced rendering with logic-tick at 12.5 fps (configurable)
- State-polled keyboard input via `SDL_GetKeyboardState`
  (fixes the input-lag issue of the terminal version)
- Same Game engine, same AI, same difficulty system, same persistence
  (stats, achievements, leaderboard) - only render + input are new
- AI demo mode (`--ai-demo`) renders the AI agent playing in the window
- Boss, UFO, power-ups all render and work
- Restart on game-over, pause, quit

What the SDL3 build does NOT support today (kept terminal-only):
- Network co-op (`--host` / `--join`)
- Replay playback / verification
- In-game level editor
- Settings menu / i18n switch

## Installing SDL3

SDL3 is the current stable line (3.2 was the first production-ready
release in early 2025). Make sure you install SDL3, not SDL2.

### Linux

```bash
# Ubuntu 24.04 LTS and later
sudo apt install libsdl3-dev

# Fedora 41+
sudo dnf install SDL3-devel

# Arch
sudo pacman -S sdl3
```

If your distribution does not yet ship SDL3 packages, build from source:

```bash
git clone --depth 1 https://github.com/libsdl-org/SDL.git
cd SDL
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build
sudo ldconfig
```

### macOS

```bash
brew install sdl3
```

### Windows with MinGW (matches the setup used for the terminal build)

Easiest path: download the prebuilt SDL3 development libraries from
`https://github.com/libsdl-org/SDL/releases` - pick the
`SDL3-devel-X.Y.Z-mingw.zip` release that matches your MinGW.

Extract it to `C:\SDL3\` (or any path; we pass it to make via
`SDL3_DIR`).

You will need to copy `SDL3.dll` next to `si_pro_sdl3.exe` after
building (or add the SDL3 `bin/` directory to your PATH). The DLL lives
under `C:\SDL3\bin\` (or `x86_64-w64-mingw32/bin/` inside the zip).

## Building

### Linux / macOS

```bash
make sdl3
./si_pro_sdl3
```

The Makefile uses `pkg-config sdl3` to find the headers and libraries.
If your SDL3 install is not on the default pkg-config path, set
`PKG_CONFIG_PATH` first:

```bash
export PKG_CONFIG_PATH=$HOME/.local/lib/pkgconfig
make sdl3
```

### Windows (MinGW)

```bash
# from the project root, with SDL3 extracted to C:\SDL3
make sdl3 SDL3_DIR=C:/SDL3

# copy the DLL alongside the exe
cp C:/SDL3/bin/SDL3.dll .

# run
si_pro_sdl3.exe
```

### CMake (any platform)

```bash
cmake -B build -DSI_BUILD_SDL3=ON
cmake --build build --target si_pro_sdl3
./build/si_pro_sdl3
```

The CMake target uses SDL3's official `find_package(SDL3 CONFIG)`
support, which works as long as SDL3 was installed via a package
manager or `cmake --install`.

## Running

```bash
./si_pro_sdl3                 # default solo, difficulty 1, random seed
./si_pro_sdl3 --ai-demo       # watch the AI play
./si_pro_sdl3 --diff 3        # nightmare difficulty
./si_pro_sdl3 --diff 0 --seed 12345 --user anurag
./si_pro_sdl3 --help
./si_pro_sdl3 --version
```

## Controls

| Key | Action |
|---|---|
| A or Left arrow | Move left |
| D or Right arrow | Move right |
| Space | Shoot |
| P | Pause |
| Q | Quit (saves stats and leaderboard) |
| R | Restart (only on game-over screen) |
| Esc | Close window |

## Architecture

The SDL3 port is purely additive - no existing terminal code was
modified. New files:

```
src/input/sdl3_keyboard.h     # IInputSource using SDL_GetKeyboardState
src/render/sdl3_renderer.h    # SDL3Renderer class - public API
src/render/sdl3_renderer.cpp  # SDL3Renderer implementation
src/main_sdl3.cpp             # SDL3 entry point (parallel to main.cpp)
```

Existing terminal code is reused entirely:

- `src/game/` - Game class, step(), render() (terminal-only)
- `src/core/` - entities, RNG, difficulty, constants
- `src/input/` - IInputSource, AISource (used by `--ai-demo` mode)
- `src/persistence/` - stats, achievements, leaderboard
- All tests still pass after the additions (verified)

This is the architectural payoff of the original design. Five lines of
public API exposure on `Game` (`step_pub`, `tick_flash_decay`,
`is_game_over`, `is_paused`, `flash_msg`) are enough to drive the
windowed loop; everything else carries over from the terminal build
unchanged.

## Why SDL3 specifically (and not SDL2)

SDL3 is now the recommended target for new C/C++ games. SDL3.2 was the
first production-ready release in early 2025, and SDL2 is in maintenance
mode. Notable benefits used in this build:

- `SDL_RenderDebugText` provides a built-in 8x8 bitmap font, eliminating
  the SDL_ttf dependency that an SDL2 port would need for HUD text.
- `SDL_GetKeyboardState` returns `const bool*` (cleaner than SDL2's
  `Uint8*`) and is what makes the lag-free movement work.
- `SDL_FRect` everywhere means rendering is naturally in subpixel
  precision without manual conversions.
- `SDL_CreateWindowAndRenderer` is a single call.

## Troubleshooting

### `SDL3/SDL.h: No such file or directory`

The compiler can't find SDL3 headers. On Windows, make sure
`SDL3_DIR=...` points to your extracted SDL3 SDK and that
`$SDL3_DIR/include/SDL3/SDL.h` actually exists. On Linux, install
`libsdl3-dev` (or your distribution's equivalent), or set
`PKG_CONFIG_PATH` so `pkg-config sdl3` resolves.

### `Package sdl3 was not found in the pkg-config search path`

Same root cause as above. Either install SDL3 from your distribution,
or after building SDL3 from source, run `sudo ldconfig` and check
`pkg-config --modversion sdl3`.

### `undefined reference to SDL_CreateWindow`

Linker can't find the SDL3 library. On Windows, check `SDL3_DIR` points
at a directory that contains `lib/libSDL3.dll.a` (or equivalent). On
Linux, make sure `pkg-config --libs sdl3` prints `-lSDL3` - if it's
empty, the package isn't installed for pkg-config to find.

### Window opens, then closes immediately on Windows

Most likely cause: `SDL3.dll` is not next to the exe. Copy it from
`<SDL3>/bin/SDL3.dll` to the same folder as `si_pro_sdl3.exe`.

### `SDL_Init failed: ...`

The error message after the colon is from SDL. The most common cause on
Linux is a headless environment with no display server - the SDL3 build
needs a graphical session (X11 or Wayland). Use the terminal version
(`./si_pro`) over SSH instead.

### Aliens move too fast / too slow

Edit `src/core/constants.h` - `FRAME_MS = 80` is the logical tick
interval. Lower = faster game, higher = slower. The SDL3 build inherits
this same constant.
