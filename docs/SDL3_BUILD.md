# SDL3 Build Quick Start

The SDL3 build is the version most players should try first. It uses the
same game simulation as the terminal build, but with a window, renderer,
audio, menus, particles, interpolation, and replay playback.

Terminal mode still matters for replay verification, LAN co-op, level
editing, localization, benchmarking, and AI tools.

## Current SDL3 Features

- Windowed rendering at 1120 x 512 logical size.
- Fixed simulation tick with interpolated rendering.
- Keyboard input through SDL3.
- SDL3 audio and simple music.
- Bosses, UFO, shields, power-ups, stats, achievements, and menus.
- AI demo mode.
- Reduced Motion setting.
- Replay recording to `<user>_last.rpl`.
- Replay playback from **Watch Replay**.

Still terminal-first:

- Replay verification: `--verify-replay`
- LAN co-op: `--host` / `--join`
- Level editor
- Hindi localization
- Benchmarking and AI training tools

## Install SDL3

Install SDL3 3.2+ development libraries.

### Linux

```bash
# Ubuntu 24.04 LTS and later
sudo apt install libsdl3-dev

# Fedora 41+
sudo dnf install SDL3-devel

# Arch
sudo pacman -S sdl3
```

If your distro does not ship SDL3 yet:

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

### Windows with MinGW

Download `SDL3-devel-X.Y.Z-mingw.zip` from:

```text
https://github.com/libsdl-org/SDL/releases
```

Extract it somewhere predictable, for example:

```text
C:\SDL3
```

After building, copy `SDL3.dll` next to `si_pro_sdl3.exe`, or add the
SDL3 `bin` directory to your PATH.

## Build

### CMake

```bash
cmake -S . -B build -DSI_BUILD_SDL3=ON
cmake --build build --target si_pro_sdl3 -j
./build/si_pro_sdl3
```

### Make on Linux/macOS

```bash
make sdl3
./si_pro_sdl3
```

If `pkg-config` cannot find SDL3:

```bash
export PKG_CONFIG_PATH=$HOME/.local/lib/pkgconfig
make sdl3
```

### Make on Windows/MinGW

```bash
make sdl3 SDL3_DIR=C:/SDL3
cp C:/SDL3/bin/SDL3.dll .
si_pro_sdl3.exe
```

## Run

```bash
./build/si_pro_sdl3
./build/si_pro_sdl3 --ai-demo
./build/si_pro_sdl3 --diff 3
./build/si_pro_sdl3 --diff 0 --seed 12345 --user anurag
./build/si_pro_sdl3 --fullscreen
./build/si_pro_sdl3 --help
./build/si_pro_sdl3 --version
```

## Replays

SDL3 writes the latest human run to:

```text
<user>_last.rpl
```

AI demo runs use:

```text
<user>_ai_last.rpl
```

These files are written to the directory you launched the game from.
If you run `./build/si_pro_sdl3` from the repo root, the replay appears
in the repo root. If you run the executable from inside `build/`, it
appears inside `build/`.

To watch a replay, open SDL3, choose **Watch Replay**, and enter the
filename. Typing `alpha_last` is enough; `.rpl` is added if the filename
has no extension.

SDL3 searches the common places that come up during development:

- the current working directory
- `build/` under the current working directory
- the parent directory
- the executable directory
- the executable directory's parent

Replay playback does not submit leaderboard scores, update lifetime
stats, or overwrite the last-run replay.

Use the terminal build for verification:

```bash
./build/si_pro --verify-replay player_last.rpl
```

## Controls

| Key | Action |
|---|---|
| A / Left Arrow | Move left |
| D / Right Arrow | Move right |
| Space | Shoot |
| P | Pause |
| Q | Quit current run |
| R | Restart from game-over screen |
| M | Toggle mute |
| F11 | Toggle fullscreen |
| Esc | Back / close |

## Troubleshooting

### `SDL3/SDL.h: No such file or directory`

Install SDL3 development headers. On Windows, check that `SDL3_DIR`
points to the extracted SDK and contains `include/SDL3/SDL.h`.

### `Package sdl3 was not found in the pkg-config search path`

Install your distro's SDL3 development package, or set
`PKG_CONFIG_PATH` to the directory containing `sdl3.pc`.

### `undefined reference to SDL_CreateWindow`

The headers were found but the library was not linked. Check your SDL3
install path. On Linux, `pkg-config --libs sdl3` should print `-lSDL3`
or equivalent linker flags.

### Window opens, then closes on Windows

`SDL3.dll` is probably missing beside the executable. Copy it from the
SDL3 SDK `bin` directory.

### `SDL_Init failed`

The SDL error after the colon is the useful part. On headless Linux or
SSH sessions, use the terminal build instead.
