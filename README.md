# Super Mario Bros

**A classic Super Mario Bros remake built in C++17 — featuring 3 levels, 2-player modes, and full browser support.**

[![Build](https://github.com/ahmed-albustany/super-mario/actions/workflows/build.yml/badge.svg)](https://github.com/ahmed-albustany/super-mario/actions/workflows/build.yml)
[![Pages](https://github.com/ahmed-albustany/super-mario/actions/workflows/pages.yml/badge.svg)](https://github.com/ahmed-albustany/super-mario/actions/workflows/pages.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS%20%7C%20Web-brightgreen.svg)](#platforms)

---

## Play Now

> **[Play in Browser](https://ahmed-albustany.github.io/super-mario)**
>
> No download required. Works on desktop and mobile browsers.

---

## Screenshots

| Gameplay | Menu |
|----------|------|
| ![Gameplay](docs/screenshots/gameplay.png) | ![Menu](docs/screenshots/menu.png) |

| HUD & World Display | Victory Screen |
|----------------------|----------------|
| ![HUD](docs/screenshots/hud.png) | ![Victory](docs/screenshots/victory.png) |

*Replace placeholder images with actual screenshots after building.*

---

## Features

### Mario Mechanics
- **Power-up system** — Small Mario, Big Mario (Mushroom), Fire Mario (Fire Flower)
- **Hit system** — Big/Fire Mario shrinks to Small on hit with invincibility frames (2-second blink)
- **Fireball shooting** — Fire Mario can shoot bouncing fireballs
- **Star invincibility** — temporary invincibility with speed boost and contact kills
- **Enemy stomping** — jump on Goombas, Koopas, and even Bowser with stomp particles and score popups
- **Koopa shells** — stomp Koopas into shells, kick them to take out other enemies
- **Question blocks** — hit from below to spawn coins, mushrooms, fire flowers, stars, or 1-ups
- **Destructible bricks** — Big Mario can break brick blocks
- **Pipes** — enterable pipes that teleport the player
- **Flagpole scoring** — height-based bonus points at the end of each level

### Levels
- **World 1-1** — Classic overworld with Goombas, Koopas, pipes, and platforming
- **World 1-2** — Underground level with more enemies, hidden blocks, and tight corridors
- **World 1-4** — Castle level with lava pits, Bowser boss fight, and castle music
- **Level progression** — complete the flagpole to advance; beat all 3 to see the Victory screen

### Game Modes
- **1 Player** — Classic single-player Mario
- **2 Player Alternating** — Players take turns; when one dies, the other plays
- **2 Player Co-op** — Both Mario and Luigi on screen simultaneously

### Visual Effects
- **Floating score popups** — "+100", "+200" text rises and fades on coin collect and enemy stomp
- **Particle effects** — coin sparkles, stomp poofs, brick debris, fireball bursts
- **Parallax backgrounds** — 3-layer scrolling for depth
- **Screen shake** — camera shake on damage
- **Invincibility blink** — sprite flashes during i-frames

### Engine
- **ECS architecture** — EnTT-based Entity Component System with 8 specialized systems
- **Scene stack** — Menu, Game, HUD, Pause, GameOver, GetReady, Victory scenes
- **Cross-platform** — native desktop (SFML) + browser (Emscripten + SDL2)
- **Mobile touch controls** — on-screen D-pad and action buttons
- **Coyote time & jump buffer** — forgiving input timing
- **Variable jump height** — hold for full height, release early for short hops

---

## Controls

### Player 1 — Keyboard

| Action | Keys |
|--------|------|
| Move Left/Right | Arrow Keys |
| Jump | Z, Space |
| Run / Fire | X, Left Shift |
| Pause | Escape, P |
| Confirm (menus) | Enter, Z |

### Player 2 — Keyboard

| Action | Keys |
|--------|------|
| Move Left/Right | A / D |
| Move Down (pipes) | S |
| Jump | J |
| Run / Fire | K |

### Mobile Touch

| Button | Position | Action |
|--------|----------|--------|
| Left / Right arrows | Bottom-left | Move |
| JUMP | Bottom-right | Jump |
| RUN | Bottom-right | Run / shoot fireballs |
| Pause (II) | Top-right | Pause menu |

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

```
super_mario/
├── src/
│   ├── core/           # Game loop, managers, config, events
│   ├── platform/       # IPlatform, SFMLPlatform, SDLPlatform
│   ├── ecs/            # Components + 8 systems
│   ├── entities/       # EntityFactory, Player, Enemy wrappers
│   ├── scenes/         # Boot, Menu, Game, Pause, HUD, GameOver, Victory
│   ├── world/          # TileMap, LevelLoader, Camera, Parallax
│   ├── physics/        # AABB math, PhysicsWorld config
│   ├── audio/          # AudioManager
│   ├── ui/             # Button, Panel, Text
│   ├── utils/          # Math, Logger, SafeFileIO, Timer
│   └── main.cpp
├── assets/
│   ├── levels/         # 3 JSON level files (1-1, 1-2, 1-4)
│   ├── textures/       # Spritesheets and backgrounds
│   ├── audio/          # Sound effects and music
│   ├── fonts/          # TTF fonts
│   └── manifest.json   # Asset manifest for preloading
├── web/                # WASM web shell (HTML, CSS, JS)
├── tests/              # Google Test unit tests
└── .github/workflows/  # CI/CD pipelines
```

---

## License

This project is licensed under the MIT License — see [LICENSE](LICENSE) for details.
