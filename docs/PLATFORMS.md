# Platform Notes

This project is meant to build on Windows, Linux, and macOS from the same C++17
source tree. The windowed build is the normal player build. The terminal binary
is kept for tools and headless checks.

## Shared Build

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

For the windowed target:

```bash
cmake -S . -B build -DSI_BUILD_SDL3=ON
cmake --build build -j
```

The window dependency must be installed before this target can link.

## Windows

MinGW:

```powershell
cmake -S . -B build -G "MinGW Makefiles" `
  -DCMAKE_MAKE_PROGRAM="C:/msys64/ucrt64/bin/mingw32-make.exe" `
  -DSI_BUILD_SDL3=ON
cmake --build build
.\build\si_pro_sdl3.exe
ctest --test-dir build --output-on-failure
```

MSVC:

```powershell
cmake -S . -B build -DSI_BUILD_SDL3=ON
cmake --build build --config Release
.\build\Release\si_pro_sdl3.exe
ctest --test-dir build --config Release --output-on-failure
```

Windows-specific terminal and socket code lives in
`src/platform/platform_win32.cpp`.

## Linux

Install a C++17 compiler, CMake, and the window dependency development package.

```bash
cmake -S . -B build -DSI_BUILD_SDL3=ON
cmake --build build -j
./build/si_pro_sdl3
ctest --test-dir build --output-on-failure
```

Linux uses `src/platform/platform_posix.cpp`.

## macOS

Install CMake and the window dependency, usually through Homebrew.

```bash
brew install cmake sdl3
cmake -S . -B build -DSI_BUILD_SDL3=ON
cmake --build build -j
./build/si_pro_sdl3
ctest --test-dir build --output-on-failure
```

macOS uses the same POSIX platform file as Linux.

## CI

The GitHub workflow builds and tests the project on Linux, Windows, and macOS.
If a local platform is not available, CI is the best quick signal.

## Portability Rule

Keep platform calls inside `src/platform/` unless there is a strong reason not
to. The rest of the code should call the small wrapper API instead of reaching
directly for Win32, POSIX, or socket setup.
