# Ruins of the Ancients

**A professional-grade 2D platformer built in C++17 — inspired by classic Super Mario Bros with original mechanics.**

[![Build](https://github.com/ahmed-albustany/super-mario/actions/workflows/build.yml/badge.svg)](https://github.com/ahmed-albustany/super-mario/actions/workflows/build.yml)
[![Pages](https://github.com/ahmed-albustany/super-mario/actions/workflows/pages.yml/badge.svg)](https://github.com/ahmed-albustany/super-mario/actions/workflows/pages.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS%20%7C%20Web-brightgreen.svg)](#platforms)

---

## Play Now

> **[▶ Play in Browser](https://ahmed-albustany.github.io/super-mario)**
>
> No download required. Works on desktop and mobile browsers.

---

## Screenshots

<!-- Replace these placeholders with actual screenshots -->

| Gameplay | Game Over |
|----------|-----------|
| ![Gameplay](docs/screenshots/gameplay.png) | ![Game Over](docs/screenshots/gameover.png) |

| Menu | HUD |
|------|-----|
| ![Menu](docs/screenshots/menu.png) | ![HUD](docs/screenshots/hud.png) |

*Screenshots are placeholders — replace with actual captures after building.*

---

## Features

- **Double Jump** — jump again mid-air for extra height and reach
- **Dash** — burst forward at high speed, break through destructible terrain, brief cooldown
- **Wall Slide & Wall Jump** — slide down walls and kick off them with velocity lockout
- **Destructible Terrain** — dash through breakable blocks to reveal secrets
- **4 Enemy Types** — Walker (patrol), Jumper (bounce), Shooter (ranged projectiles), Guardian (armored, 2 hits)
- **3 Collectible Types** — Coins (score), Gem Shards (rare), Power Crystals (invincibility + speed boost)
- **Dynamic HUD** — live score, lives, coins, gems, countdown timer, power-up duration bar
- **Parallax Backgrounds** — 3-layer scrolling ruins with depth effect
- **Screen Shake** — camera shake on damage for impact feedback
- **Coyote Time & Jump Buffer** — forgiving input timing for tight platforming
- **Variable Jump Height** — release early for short hops, hold for full height
- **Arcade Rules** — no saves, no file writes, session-only score and lives
- **Cross-Platform** — native desktop (SFML) + browser (Emscripten + SDL2)
- **Mobile Touch Controls** — on-screen D-pad and action buttons for phones/tablets

---

## Controls

### Keyboard

| Action | Keys |
|--------|------|
| Move Left/Right | Arrow Keys, A/D |
| Jump | Z, Space, W |
| Dash | X, Left Shift |
| Pause | Escape, P |
| Confirm | Enter, Z |
| Debug Overlay | F1 |

### Mobile Touch

| Button | Position | Action |
|--------|----------|--------|
| Left Arrow | Bottom-left | Move left |
| Right Arrow | Bottom-left | Move right |
| JUMP | Bottom-right | Jump / double jump |
| DASH | Bottom-right | Dash |
| Pause | Top-right | Pause menu |

---

## Download

Pre-built binaries are available on the [Releases](https://github.com/ahmed-albustany/super-mario/releases) page:

| Platform | Download |
|----------|----------|
| Windows | `MarioGame-windows.zip` |
| Linux | `MarioGame-linux.tar.gz` |
| macOS | `MarioGame-macos.zip` |

Or [play in the browser](https://ahmed-albustany.github.io/super-mario) — no download needed.

---

## Building from Source

### Prerequisites

- **CMake 3.20+**
- **C++17 compiler** (MSVC 2019+, GCC 11+, Clang 14+)
- **Emscripten SDK** (for WASM builds only)

All C++ dependencies are fetched automatically via CMake FetchContent.

### Native Builds

```bash
# Debug (with sanitizers on GCC/Clang)
cmake --preset debug-native
cmake --build build/debug-native

# Release (optimized)
cmake --preset release-native
cmake --build build/release-native
```

### WASM Builds

```bash
# Requires Emscripten activated: source emsdk_env.sh

# Debug (with assertions)
cmake --preset debug-wasm
cmake --build build/debug-wasm

# Release (optimized + closure compiler)
cmake --preset release-wasm
cmake --build build/release-wasm
```

### Running Tests

```bash
cmake --preset debug-native
cmake --build build/debug-native
cd build/debug-native
ctest --output-on-failure
```

See [docs/BUILD.md](docs/BUILD.md) for detailed platform-specific instructions.

---

## Dependencies

| Library | Version | Purpose | Fetched By |
|---------|---------|---------|------------|
| [SFML](https://www.sfml-dev.org/) | 2.6.1 | Native rendering, audio, input | CMake FetchContent |
| [SDL2](https://www.libsdl.org/) | 2.x | WASM rendering, audio, input | Emscripten ports |
| [EnTT](https://github.com/skypjack/entt) | 3.13.2 | Entity Component System | CMake FetchContent |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.3 | Level and manifest parsing | CMake FetchContent |
| [Google Test](https://github.com/google/googletest) | 1.14.0 | Unit testing | CMake FetchContent |

---

## Architecture

The engine uses a **Platform Abstraction Layer** so game code never touches SFML or SDL directly. All rendering, input, and audio go through `IPlatform` — the concrete implementation is selected at compile time (SFML for native, SDL2 for WASM).

Game logic is driven by an **Entity Component System** (EnTT) with 8 specialized systems processed in a fixed order each frame. Scenes are managed via a **stack-based scene manager** with deferred push/pop/replace commands.

For the full technical breakdown, see [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

---

## Project Structure

```
super_mario/
├── src/
│   ├── core/           # Game loop, managers, config, events
│   ├── platform/       # IPlatform, SFMLPlatform, SDLPlatform
│   ├── ecs/            # Components + 8 systems
│   ├── entities/       # EntityFactory, Player, Enemy wrappers
│   ├── scenes/         # Boot, Menu, Game, Pause, HUD, GameOver
│   ├── world/          # TileMap, LevelLoader, Camera, Parallax
│   ├── physics/        # AABB math, PhysicsWorld config
│   ├── audio/          # AudioManager
│   ├── ui/             # Button, Panel, Text
│   ├── utils/          # Math, Logger, SafeFileIO, Timer, StateMachine
│   └── main.cpp
├── assets/
│   ├── levels/         # JSON level files
│   ├── textures/       # Spritesheets and backgrounds
│   ├── audio/          # Sound effects and music
│   ├── fonts/          # TTF fonts
│   └── manifest.json   # Asset manifest for preloading
├── web/                # WASM web shell (HTML, CSS, JS)
├── tests/              # Google Test unit tests
├── docs/               # Architecture and build documentation
└── .github/workflows/  # CI/CD pipelines
```

---

## License

This project is licensed under the MIT License — see [LICENSE](LICENSE) for details.
