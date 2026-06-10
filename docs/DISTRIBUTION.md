# Distribution

This is the current release packaging plan. It is intentionally simple:
ship what is validated, mark what is not, and avoid pretending the SDL3
runtime story is solved on every platform before it is.

## Release Workflow

Release packaging lives in:

```text
.github/workflows/release.yml
```

It supports:

- manual runs with a version string
- tags matching `v*`
- CMake builds
- tests before packaging
- Windows, Linux, macOS, and website artifacts
- optional GitHub Release publication

## Packages

### Windows Terminal Package

```text
space-invaders-pro-windows-x64.zip
```

Contents:

```text
si_pro.exe
README.md
CHANGELOG.md
LICENSE
docs/
```

### Windows SDL3 Package

Target name:

```text
space-invaders-pro-sdl3-windows-x64.zip
```

Expected contents:

```text
si_pro_sdl3.exe
SDL3.dll
README.md
CHANGELOG.md
LICENSE
docs/SDL3_BUILD.md
```

Status: needs CI/runtime validation for `SDL3.dll` bundling.

### Linux Terminal Package

```text
space-invaders-pro-linux-x64.tar.gz
```

Contents:

```text
si_pro
README.md
CHANGELOG.md
LICENSE
docs/
```

### Linux SDL3 Package

Target name:

```text
space-invaders-pro-sdl3-linux-x64.tar.gz
```

Expected contents:

```text
si_pro_sdl3
README.md
CHANGELOG.md
LICENSE
docs/SDL3_BUILD.md
```

Status: decide whether the package expects system `libsdl3` or bundles
runtime libraries.

### macOS Terminal Package

```text
space-invaders-pro-macos-universal.tar.gz
```

Contents:

```text
si_pro
README.md
CHANGELOG.md
LICENSE
docs/
```

### macOS SDL3 Package

Target:

```text
Space Invaders Pro Edition.app
```

Status: needs app bundling, signing/notarization decision, and runtime
validation. Until then, macOS releases should use the terminal package.

### Website Package

```text
space-invaders-pro-website.zip
```

Contents:

```text
website/index.html
website/styles.css
website/README.md
```

The website is static and has no build step.

## Release Checklist

Before publishing:

- [ ] README, CHANGELOG, ROADMAP, and SDL3 build notes are current.
- [ ] Normal CI is green.
- [ ] Release workflow was run on the target commit.
- [ ] Tests passed in release workflow logs.
- [ ] Downloaded packages smoke-tested locally.
- [ ] `si_pro --version` works in terminal packages.
- [ ] Website links point to the intended repository/release.
- [ ] SDL3 packages are clearly marked experimental until runtime
      packaging is validated.

## Current Download Wording

Use this until SDL3 packages are validated:

> Download the terminal tools package for the validated cross-platform
> build. It includes replay verification, LAN co-op, level editing,
> benchmarking, and AI tooling. The SDL3 build is available from source
> and will get packaged releases once runtime bundling is tested.

After SDL3 packages are validated:

> Download the SDL3 build if you want to play. Download the terminal
> tools package if you need replay verification, LAN co-op, level
> editing, benchmarking, or AI tooling.

## Later

- Add validated SDL3 packages to the release workflow.
- Add a Windows package smoke test for `SDL3.dll`.
- Document Linux SDL3 runtime dependency clearly.
- Add macOS `.app` generation.
- Add checksums for release assets.
