# Project Website

This folder contains the Phase 2 static website for Space Invaders -
Pro Edition.

## Preview

Open `website/index.html` directly in a browser. No build step, package
manager, or dev server is required.

## Intended Sections

- Hero
- Game trailer
- Screenshots
- Gameplay features
- Controls
- Download latest build
- GitHub repository
- Technical architecture
- Development journey
- Roadmap
- Contributors
- Changelog
- FAQ
- How to contribute

## Media Plan

The first version uses authored visual frames so the page has a complete
layout before capture assets exist. For release, replace the placeholder
trailer and screenshot cards with real SDL3 media:

- `hero-gameplay.webp` - short, high-contrast gameplay still.
- `trailer-poster.webp` - trailer thumbnail.
- `screenshot-menu.webp` - SDL3 main menu.
- `screenshot-gameplay.webp` - normal wave with power-up.
- `screenshot-boss.webp` - boss warning or active boss phase.
- `screenshot-replay-viewer.webp` - browser replay viewer.

Keep image dimensions stable so layout does not shift on load.
