# PIXEL RUSH

**A 4-player fruit-collection platformer built in C++17 — 10 levels, 5 game modes, and full browser support.**

[![Build](https://github.com/ahmed-albustany/super-mario/actions/workflows/build.yml/badge.svg)](https://github.com/ahmed-albustany/super-mario/actions/workflows/build.yml)
[![Pages](https://github.com/ahmed-albustany/super-mario/actions/workflows/pages.yml/badge.svg)](https://github.com/ahmed-albustany/super-mario/actions/workflows/pages.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS%20%7C%20Web-brightgreen.svg)](#platforms)

---

## Play Now

> **[Play in Browser](https://ahmed-albustany.github.io/super-mario/)**
>
> No download required. Works on desktop and mobile browsers.

---

## Screenshots

| Gameplay | Level Select |
|----------|--------------|
| ![Gameplay](docs/screenshots/gameplay.png) | ![Level Select](docs/screenshots/level_select.png) |

| Multiplayer | Victory |
|-------------|---------|
| ![Multiplayer](docs/screenshots/multiplayer.png) | ![Victory](docs/screenshots/victory.png) |

> **Adding screenshots:** Build and run the game, then capture screenshots and save them to `docs/screenshots/`. Recommended resolution: 1280x720. Use PNG format for best quality.

---

## How to Play

1. **Move** with Arrow Keys (or WASD for Player 2)
2. **Jump** with Z or Space — hold for a higher jump, tap for a short hop
3. **Double Jump** by pressing X in mid-air
4. **Run** by holding X while moving for extra speed
5. **Collect fruits** scattered through each level for points
6. **Activate checkpoints** by touching flag markers — you'll respawn there on death
7. **Reach the trophy** at the end of each level to advance
8. **Avoid traps** — saws, spikes, fire, and more will cost you a life
9. Complete all 10 levels across 5 worlds to win!

---

## Features

### Gameplay
- **4 playable characters** — Mask Dude, Ninja Frog, Pink Man, Virtual Guy
- **Fruit collection** — 8 fruit types (cherry, apple, orange, pineapple, melon, strawberry, kiwi, banana) with increasing point values
- **Precision platforming** — smooth acceleration/deceleration, variable jump height, coyote time, and jump buffering
- **Double jumping & wall jumping** — fluid aerial movement
- **Checkpoint system** — activate checkpoints to save progress within each level
- **Death animation** — character flips and hops off screen, then respawns after a brief delay
- **Level complete screen** — score summary with time bonus before advancing
- **Trophy goals** — reach the end-of-level trophy to advance
- **Victory screen** — congratulations and final score after completing all 10 levels

### Levels
- **10 hand-crafted levels** across 5 worlds with progressive difficulty
- **World 1 (1-1, 1-2)** — Green overworld: tutorial platforming, moving saws
- **World 2 (2-1, 2-2)** — Blue underground: fire traps, falling platforms
- **World 3 (3-1, 3-2)** — Purple castle: spike heads, rock heads
- **World 4 (4-1, 4-2)** — Pendulum peril & wind tunnels: spiked balls, fans
- **World 5 (5-1, 5-2)** — Final gauntlet: every trap type combined at maximum density
- **Level select screen** — choose any level before starting

### Traps & Hazards
- Saws (rotating, moving), spikes, fire traps (timed on/off)
- Spike heads (charge at players), rock heads (falling)
- Spiked balls (pendulum swing), fans (wind push)
- Falling platforms, moving platforms, trampolines, arrows

### Game Modes
- **Solo** — Single-player adventure
- **2P Alternating** — Take turns on death
- **2P Co-op** — Both players on screen simultaneously
- **4P Co-op** — Four players cooperating on screen
- **4P VS** — Four players competing for the highest fruit score

### HUD
- Score display with live updates on fruit collection
- Lives counter with character icon
- World/level indicator
- Countdown timer with red warning under 100 seconds
- Per-player score panels in multiplayer modes

### Engine
- **ECS architecture** — EnTT-based Entity Component System
- **Scene stack** — Boot, Menu, LevelSelect, Game, HUD, Pause, GameOver, GetReady, LevelComplete, Victory
- **Cross-platform** — native desktop (SFML 2.6) + browser (Emscripten + SDL2)
- **Mobile touch controls** — on-screen D-pad and action buttons
- **Parallax backgrounds** — 3-layer scrolling for depth
- **Particle effects** — fruit sparkles, stomp poofs, brick debris

---

## Controls

### Player 1 — Keyboard

| Action | Keys |
|--------|------|
| Move Left/Right | Arrow Keys |
| Jump | Z, Space |
| Double Jump / Run | X, Left Shift |
| Pause | Escape, P |
| Confirm (menus) | Enter, Z |

### Player 2 — Keyboard

| Action | Keys |
|--------|------|
| Move Left/Right | A / D |
| Jump | J |
| Double Jump / Run | K |

### Player 3 — Keyboard

| Action | Keys |
|--------|------|
| Move Left/Right | Numpad 4 / 6 |
| Jump | Numpad 8 |
| Double Jump / Run | Numpad 5 |

### Player 4 — Keyboard

| Action | Keys |
|--------|------|
| Move Left/Right | Numpad Left / Right |
| Jump | Numpad 1 |
| Double Jump / Run | Numpad 2 |

### Mobile Touch

| Button | Position | Action |
|--------|----------|--------|
| Left / Right arrows | Bottom-left | Move |
| JUMP | Bottom-right | Jump |
| RUN | Bottom-right | Run / dash |
| Pause (II) | Top-right | Pause menu |

---

## Download

Pre-built binaries are available on the [Releases](https://github.com/ahmed-albustany/super-mario/releases) page:

| Platform | Download |
|----------|----------|
| Windows | `PixelRush-windows.zip` |
| Linux | `PixelRush-linux.tar.gz` |
| macOS | `PixelRush-macos.zip` |

Or [play in the browser](https://ahmed-albustany.github.io/super-mario/) — no download needed.

---

## Building from Source

### Prerequisites

- **CMake 3.20+**
- **C++17 compiler** (MSVC 2019+, GCC 11+, Clang 14+)
- **Emscripten SDK** (for WASM builds only)

All C++ dependencies are fetched automatically via CMake FetchContent.

### Windows

```bash
cmake --preset debug-native
cmake --build build/debug-native
./build/debug-native/MarioGame.exe
```

### Linux

```bash
cmake --preset debug-native
cmake --build build/debug-native
./build/debug-native/MarioGame
```

### macOS

```bash
cmake --preset debug-native
cmake --build build/debug-native
./build/debug-native/MarioGame
```

### Release Build (all platforms)

```bash
cmake --preset release-native
cmake --build build/release-native
```

### WASM / Browser Build

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

## Technology Stack

| Technology | Version | Purpose |
|------------|---------|---------|
| **C++17** | — | Core language |
| [SFML](https://www.sfml-dev.org/) | 2.6.1 | Native rendering, audio, input |
| [SDL2](https://www.libsdl.org/) | 2.x | WASM rendering, audio, input (via Emscripten ports) |
| [EnTT](https://github.com/skypjack/entt) | 3.13.2 | Entity Component System |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.3 | Level and manifest parsing |
| [Google Test](https://github.com/google/googletest) | 1.14.0 | Unit testing |
| [Emscripten](https://emscripten.org/) | 3.x | C++ to WebAssembly compilation |

---

## Architecture

The engine uses a **Platform Abstraction Layer** so game code never touches SFML or SDL directly. All rendering, input, and audio go through `IPlatform` — the concrete implementation is selected at compile time (SFML for native, SDL2 for WASM).

Game logic is driven by an **Entity Component System** (EnTT) with specialized systems processed in a fixed order each frame. Scenes are managed via a **stack-based scene manager** with deferred push/pop/replace commands.

```
pixel_rush/
├── src/
│   ├── core/           # Game loop, managers, config, events
│   ├── platform/       # IPlatform, SFMLPlatform, SDLPlatform
│   ├── ecs/            # Components + 10 systems
│   ├── entities/       # EntityFactory, Player wrappers
│   ├── scenes/         # Boot, Menu, LevelSelect, Game, Pause, HUD, LevelComplete, GameOver, GetReady, Victory
│   ├── world/          # TileMap, LevelLoader, Camera, Parallax
│   ├── physics/        # AABB math, PhysicsWorld config
│   ├── audio/          # AudioManager (graceful fallback on missing sounds)
│   ├── ui/             # Button, Panel, Text
│   ├── utils/          # Math, Logger, SafeFileIO, Timer
│   └── main.cpp
├── assets/
│   ├── levels/         # 10 JSON level files (1-1 through 5-2)
│   ├── textures/       # Sprite sheets and backgrounds
│   ├── audio/          # Sound effects and music (OGG)
│   ├── fonts/          # TTF fonts
│   └── manifest.json   # Asset manifest for preloading
├── web/                # WASM web shell (HTML, CSS, JS, touch controls)
├── tests/              # Google Test unit tests
└── .github/workflows/  # CI/CD pipelines
```

---

## License

This project is licensed under the MIT License — see [LICENSE](LICENSE) for details.
