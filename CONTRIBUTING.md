# Contributing

Thanks for taking a look. This project started as my final-year project,
but I am maintaining it as an open-source game now. Good changes should
make the game easier to play, build, test, package, or understand.

## Before You Change Code

- Check [ROADMAP.md](ROADMAP.md) for the current direction.
- Keep changes focused. One feature, fix, or cleanup per pull request is
  much easier to review.
- Do not mix gameplay changes, rendering changes, docs, and refactors
  unless they are part of the same problem.
- If you touch deterministic systems, think about replay compatibility.

The most sensitive areas are RNG usage, simulation order, action masks,
replay parsing, level transitions, and collision behavior.

## Build And Test

Terminal/tools build:

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

SDL3 build:

```bash
cmake -S . -B build -DSI_BUILD_SDL3=ON
cmake --build build --target si_pro_sdl3 -j
```

SDL3 setup notes are in [docs/SDL3_BUILD.md](docs/SDL3_BUILD.md).

## Where To Start

| Area | Files |
|---|---|
| Core simulation | `src/game/game.h`, `src/game/game_step.cpp` |
| SDL3 app | `src/main_sdl3.cpp`, `src/render/sdl3_renderer.cpp` |
| Terminal app/tools | `src/main.cpp`, `src/ui/menus.cpp`, `src/ui/tools.cpp` |
| Input | `src/input/input_source.h` |
| AI player | `src/input/ai_source.cpp` |
| Director AI | `src/director/director.cpp` |
| Replay format | `src/persistence/replay_file.cpp` |
| Level format | `src/persistence/level_file.cpp` |
| Tests | `tests/` |
| Architecture notes | `docs/ARCHITECTURE.md` |

## Testing Expectations

Run the full test suite before submitting changes.

Add or update tests when practical, especially for:

| Change area | Test focus |
|---|---|
| RNG or seeded behavior | Determinism |
| Replay files | Round-trip / verification |
| Level files | Parser and writer behavior |
| AI input | Action masks and profiles |
| Simulation rules | Snapshot or scenario tests |
| Persistence | Read/write compatibility |

Rendering, audio, networking, and menus may still need manual testing.
If so, write down what you checked.

## Pull Request Checklist

- [ ] The change has one clear purpose.
- [ ] Existing tests pass.
- [ ] New behavior is tested where practical.
- [ ] User-facing docs are updated when behavior changes.
- [ ] SDL3 and terminal behavior are both considered.
- [ ] Replay/determinism impact has been considered.

## Good First Areas

- Small docs fixes where current behavior is unclear.
- Scenario tests for power-ups, boss phases, collisions, or level
  transitions.
- SDL3 troubleshooting notes from real setup issues.
- Issue templates or release checklist improvements.

Please open an issue or write a short design note before starting a
large gameplay or architecture rewrite.
