# Cross-Platform Build Notes

This document explains how `si_pro` compiles on three operating systems
from one source tree, and how contributors or release maintainers can
verify each platform independently.

The short version: there is **one** `#ifdef _WIN32` in the entire
codebase, in `src/platform/platform.h`. Everything else is portable
C++17 plus POSIX. Both macOS and Linux compile the same code; Windows
compiles a parallel set of system-call wrappers.

---

## The conditional-compilation strategy

All platform differences are isolated in two files:

```
src/platform/platform.h          ← public API, used by everyone
src/platform/platform_posix.cpp  ← Linux + macOS implementation
src/platform/platform_win32.cpp  ← Windows implementation
```

The build system picks **one** of the two `.cpp` files at link time:

- `Makefile` auto-detects via `$(OS)` and picks the right file.
- `CMakeLists.txt` does the same via `if(WIN32)`.
- Direct `g++` invocation: pass the file you want explicitly.

### What `platform.h` exposes

The public API surface is intentionally small — about a dozen
functions, all in the `si::platform` namespace:

```cpp
namespace si::platform {
    // Terminal mode (non-canonical, no-echo) for raw keyboard reads
    void enable_raw_mode();
    void disable_raw_mode();

    // Non-blocking keyboard input
    bool kb_available();
    int  read_key();

    // Timing
    void sleep_ms(int ms);
    std::int64_t now_ms();

    // Network init (no-op on POSIX, WSAStartup/WSACleanup on Windows)
    void net_init();
    void net_cleanup();

    // Cross-platform errno
    int socket_errno();
    std::string socket_errstr(int err);
}
```

Every other module in the codebase calls only these functions. Nothing
else in the project touches the operating system directly.

### What's `#ifdef`-ed in the header

The header itself has a small platform-switched block — this is the
**only** `#ifdef _WIN32` in the entire project:

```cpp
// src/platform/platform.h
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <winsock2.h>
    using socket_t = SOCKET;
    inline constexpr socket_t INVALID_SOCK = INVALID_SOCKET;
    #define SI_CLEAR_CMD "cls"
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    using socket_t = int;
    inline constexpr socket_t INVALID_SOCK = -1;
    #define SI_CLEAR_CMD "clear"
#endif
```

That's it. Every higher-level file in the project (`src/game/`,
`src/input/`, `src/net/`, etc.) just includes `platform.h` and uses
`socket_t`, `SI_CLEAR_CMD`, and the `si::platform::*` functions without
caring which OS it's running on.

---

## What compiles on each platform

### Linux

| Component | Header / API | Source |
|---|---|---|
| Terminal mode | `<termios.h>` (POSIX) | `platform_posix.cpp` |
| Non-blocking key | `select()` + `read()` | `platform_posix.cpp` |
| Sleep | `usleep` / `nanosleep` | `platform_posix.cpp` |
| Sockets | BSD `<sys/socket.h>` | `platform_posix.cpp` |
| Threading | `pthread` (via `<thread>`) | linked with `-pthread` |
| Clear screen | `system("clear")` | macro `SI_CLEAR_CMD` |

Tested directly on Ubuntu 24 with g++ 11.4 throughout development.
This is the platform I have the most evidence for: all 11,510 test
assertions pass, the binary runs interactively, loopback co-op works,
all CLI modes (`--benchmark`, `--evolve-ai`, `--verify-replay`,
`--train-ai`, `--version`, `--ai-demo`) all run end-to-end.

### macOS

| Component | Header / API | Source |
|---|---|---|
| Terminal mode | `<termios.h>` (POSIX, identical to Linux) | `platform_posix.cpp` |
| Non-blocking key | `select()` + `read()` (identical) | `platform_posix.cpp` |
| Sleep | `usleep` / `nanosleep` (identical) | `platform_posix.cpp` |
| Sockets | BSD sockets — **invented on BSD Unix**, macOS's ancestor | `platform_posix.cpp` |
| Threading | `pthread` (Apple's implementation, same API) | linked with `-pthread` |
| Clear screen | `system("clear")` | macro `SI_CLEAR_CMD` |

macOS compiles **the same** `platform_posix.cpp` as Linux. Every
header listed above exists on macOS with the same signatures and the
same semantics. There is no macOS-specific code in this project.

I have not personally run a build on a Mac during development. What I
can say with confidence:

1. The `.github/workflows/build.yml` CI workflow includes a
   `macos-latest` runner that builds with Apple Clang and runs the
   test suite. Push to GitHub and the matrix verifies this in ~5 min.
2. Every API used in `platform_posix.cpp` is POSIX-standardised and
   present on macOS — none of them are Linux-specific (no `epoll`, no
   `inotify`, no `/proc`).
3. If you don't have a Mac, see the verification instructions below.

#### Possible Clang-vs-GCC differences

Apple Clang is slightly stricter than GCC on a few warnings. The
Makefile uses `-Wall -Wextra -Wpedantic` but **does not use
`-Werror`**, so warnings won't break the build. Most likely candidates
if Clang complains:

- Unused-parameter warnings on `IInputSource::poll(uint32_t tick, ...)`
  in implementations that don't use `tick` (already silenced via
  `(void)tick;` where applicable).
- Sign-comparison warnings in `for (int i = 0; i < v.size(); i++)`
  style loops — these are mostly already converted to `size_t`.
- Implicit narrowing where the code does `int x = vec.size()` — these
  appear in test files but not the main binary.

If you see Clang warnings I missed, they are cosmetic; the build
will still produce a working binary.

### Windows

| Component | Header / API | Source |
|---|---|---|
| Terminal mode | Win32 console API (`SetConsoleMode`) | `platform_win32.cpp` |
| Non-blocking key | `_kbhit()` + `_getch()` from `<conio.h>` | `platform_win32.cpp` |
| Sleep | `Sleep()` from `<windows.h>` | `platform_win32.cpp` |
| Sockets | Winsock2 (`<winsock2.h>`, `<ws2tcpip.h>`) | `platform_win32.cpp` |
| Threading | Win32 threads (via `<thread>`, no `-pthread`) | linked with `-lws2_32` |
| Clear screen | `system("cls")` | macro `SI_CLEAR_CMD` |

`platform_win32.cpp` is the only file in the codebase that includes
`<windows.h>` and `<conio.h>`. It exposes exactly the same
`si::platform::*` API as the POSIX file — same function signatures,
same return types. Higher layers don't know which file they're
linked against.

#### One Windows-specific pitfall (already handled)

`<windows.h>` defines `ERROR` and `DEBUG` as preprocessor macros,
which would corrupt the `LogLevel::ERR` and `LogLevel::DBG` enum
values used in `src/debug/logger.h`. The codebase handles this in
three layers of defence:

1. `platform.h` defines `WIN32_LEAN_AND_MEAN` and `NOMINMAX` before
   including `<winsock2.h>` — strips most macro pollution.
2. `logger.h` issues `#undef ERROR` and `#undef DEBUG` defensively
   right before the enum definition.
3. The enum values themselves are `DBG` and `ERR` (renamed away from
   the colliding identifiers); the user-facing macros `LOG_DEBUG`
   and `LOG_ERROR` keep their old names by mapping to the new enum.

This is the standard cross-platform C++ workaround and is worth keeping
in mind whenever platform headers are included.

---

## How to verify each platform yourself

### Linux

```bash
git clone <repo-url> si_pro
cd si_pro
make
./si_pro --version
make run-tests          # all 11,510 assertions should pass
```

If you don't have GCC 11+: `sudo apt install g++` (Ubuntu/Debian) or
your distribution's equivalent.

### macOS

```bash
git clone <repo-url> si_pro
cd si_pro
make                    # uses Apple Clang via /usr/bin/c++
./si_pro --version
make run-tests
```

If `make` fails because of GNU Make 3.81 (shipped with macOS): install
a newer Make via Homebrew and use `gmake` instead:

```bash
brew install make
gmake
gmake run-tests
```

Or use CMake, which doesn't depend on which Make is installed:

```bash
cmake -S . -B build
cmake --build build -j
./build/si_pro --version
cd build && ctest          # if tests are wired in CMake
```

If neither works on your Mac, open an issue or pull request with the
exact `make` or CMake error. Most likely fixes are small portability
tweaks exposed by Apple Clang.

### Windows

Two paths depending on what's installed:

**With MinGW:**
```powershell
cd si_pro
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc `
    src/main.cpp `
    src/core/*.cpp src/config/*.cpp src/debug/*.cpp `
    src/render/*.cpp src/persistence/*.cpp src/net/*.cpp `
    src/input/*.cpp src/game/*.cpp src/editor/*.cpp src/ui/*.cpp `
    src/i18n/*.cpp src/platform/platform_win32.cpp `
    -o si_pro.exe -lws2_32
.\si_pro.exe --version
```

**With CMake + MinGW:**
```powershell
cmake -S . -B build -G "MinGW Makefiles" `
    -DCMAKE_MAKE_PROGRAM="C:/msys64/ucrt64/bin/mingw32-make.exe"
cmake --build build
.\build\si_pro.exe --version
```

**With Visual Studio (MSVC):**
```powershell
cmake -S . -B build              # MSVC is CMake's default on Windows
cmake --build build --config Release
.\build\Release\si_pro.exe --version
```

The interactive game runs in Windows Terminal, the new Windows 11
console, and the legacy `cmd.exe` console. UTF-8 (for the Hindi UI)
works in Windows Terminal and modern conhost; older conhost may show
garbage characters in place of Devanagari. This is documented behaviour
and not a bug in the project — it's a terminal limitation.

### CI (the easiest verification of all)

Push the project to a public GitHub repository. The
`.github/workflows/build.yml` workflow runs automatically and builds
on all three platforms in parallel. Five minutes after push, the
green/red badges in the Actions tab tell you definitively whether
each platform builds and passes tests.

For a private repo, the same CI runs but with monthly minutes limits;
on the free tier those limits are generous enough for a student
project.

---

## Confidence summary

| Platform | Build verified | Tests run | Interactive run | CI configured |
|---|---|---|---|---|
| Linux (Ubuntu, g++ 11/12) | ✅ Yes, throughout development | ✅ All 11,510 pass | ✅ Yes | ✅ Yes (matrix) |
| Windows (MinGW) | ✅ Built by author | ⚠️ Author confirms binary runs | ✅ See demo video | ✅ Yes (workflow) |
| macOS (Apple Clang) | ❌ Not on real hardware | ❌ Not directly | ❌ Not directly | ✅ Yes (workflow) |

The macOS row is the honest gap. If a contributor has a Mac and runs the
commands listed above, that result should be captured in an issue, pull
request, or release note. The CI matrix is set up to catch platform
problems on push to GitHub.

The codebase contains no Linux-specific APIs, no Mac-specific APIs,
no Windows-specific APIs outside `platform_win32.cpp`. Every
operating-system call goes through the abstraction layer. That is the
architectural guarantee that makes "should work on macOS" a defensible
claim rather than a hopeful one.
