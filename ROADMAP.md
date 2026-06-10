# Roadmap

This roadmap tracks how I am moving the game from a final-year project
into something I can keep maintaining in the open. It is not a promise
to add every possible feature. The aim is to keep the game playable,
understandable, and worth showing.

## Done

### Project Identity

- README rewritten around the current project, not the original college
  submission.
- Academic history moved into an Origins section.
- `CONTRIBUTING.md`, `ROADMAP.md`, and `CHANGELOG.md` added.
- SDL3 positioned as the build most players should try first.
- Terminal mode kept as the tools/developer build.

### Website And Distribution

- Static website added in `website/`.
- Release packaging plan added in `docs/DISTRIBUTION.md`.
- GitHub release workflow added at `.github/workflows/release.yml`.
- Current packages focus on validated terminal builds while SDL3 runtime
  packaging is still being tightened.

### Signature Gameplay

- Director Beats selected as the main identity feature.
- Director can surface Relief Window and Pressure Surge beats.
- SDL3 HUD shows active beat state.
- Director behavior covered by `tests/test_director.cpp`.

### Architecture Hardening

- `src/core/game_event.h` added.
- `Game` exposes a per-tick gameplay event buffer.
- Events now cover bullet fire, alien kills, power-up collection, boss
  phase changes, player hits, and level clears.
- SDL3 audio and Director AI can use events instead of only state diffs.
- Event behavior covered by `tests/test_events.cpp`.

### Gameplay / Accessibility

- SDL3 Reduced Motion setting added.
- `sdl3.reduced_motion` config key added.
- Screen shake is disabled when Reduced Motion is on.
- Config behavior covered by `tests/test_config.cpp`.

### Replay Work

- SDL3 can watch replay files from the main menu.
- SDL3 now writes `<user>_last.rpl` for human runs and
  `<user>_ai_last.rpl` for AI demo runs.
- Replay verification remains a terminal/headless workflow.

## Next

These are the areas I would work on next, in order.

1. **SDL3 release packaging**
   - Validate Windows package with `SDL3.dll`.
   - Document Linux SDL3 runtime dependency clearly.
   - Decide whether macOS ships as a raw binary first or waits for an
     `.app` bundle.

2. **Replay presentation**
   - Make replay playback easier to discover and use.
   - Consider a replay browser instead of typing filenames.
   - Keep replay verification headless and scriptable.

3. **Gameplay content pass**
   - Add wave archetypes or boss variants only after writing a small
     design note.
   - Avoid adding several new mechanics at once.

4. **Input polish**
   - Controller support.
   - Remapping or a clearer controls/settings screen.

5. **Docs and contributor workflow**
   - Keep README, SDL3 build notes, and distribution docs aligned with
     actual behavior.
   - Add issue templates once the public contribution flow settles.

## Deferred Ideas

| Idea | Why deferred |
|---|---|
| Replay ghosts | Interesting, but needs careful UI and determinism work |
| Modifier cards | Could be fun, but risks feature bloat |
| Boss variants | Worth doing after the current boss identity is clearer |
| SDL3 LAN co-op | Useful, but networking UI needs a separate pass |
| Full localization in SDL3 | Needs a better text/font path than debug text |

## Maintenance Rules

- Keep CI green.
- Add tests when deterministic behavior changes.
- Keep replay compatibility in mind.
- Prefer small pull requests.
- Update `CHANGELOG.md` before release.
