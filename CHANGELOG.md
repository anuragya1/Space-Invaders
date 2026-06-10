# Changelog

Notable changes to Space Invaders - Pro Edition.

## Unreleased

### Changed

- README and contributor docs now present the project as an ongoing
  open-source game instead of an academic submission first.
- SDL3 is documented as the build most players should try first.
- Terminal mode is documented as the tools/developer build.
- Director AI now exposes visible Relief Window and Pressure Surge beats.
- SDL3 audio and Director AI can consume explicit gameplay events, with
  state-diff fallback kept where needed.
- SDL3 Reduced Motion disables screen shake without changing gameplay.
- SDL3 replay playback is available from the main menu.
- SDL3 writes `<user>_last.rpl` for human runs and
  `<user>_ai_last.rpl` for AI demo runs.

### Added

- `CONTRIBUTING.md`
- `ROADMAP.md`
- `docs/DISTRIBUTION.md`
- Static website in `website/`
- Release packaging workflow at `.github/workflows/release.yml`
- Director Beats and SDL3 HUD presentation
- `src/core/game_event.h`
- Gameplay events for bullet fire, alien kills, power-up collection,
  boss phase changes, player hits, and level clears
- Config support and tests for `sdl3.reduced_motion`
- SDL3 **Watch Replay** screen
- Tests for Director behavior, gameplay events, and config persistence

### Compatibility

- Replay file format is unchanged.
- Save file format is unchanged.
- CLI behavior is unchanged.
- Replay playback in SDL3 uses temporary replay stats and does not submit
  scores to the leaderboard.

## Existing Foundation

- SDL3 renderer, audio, menus, stats, achievements, replay recording,
  and replay playback.
- Terminal replay playback/verification, LAN co-op, localization, level
  editor, benchmarking, AI training, and GA tuning.
- Deterministic simulation using seeded RNG and input masks.
- Human-readable replay files with RLE compression.
- Boss encounters, UFO, shields, combo scoring, and power-ups.
- CI across Linux, macOS, and Windows.
- Tests for RNG, replay, level files, AI, Director, events, config, and
  determinism.
