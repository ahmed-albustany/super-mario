# Build Instructions

Step-by-step build guide for **Ruins of the Ancients** on all supported platforms.

---

## Prerequisites

### All Platforms

| Tool | Version | Purpose |
|------|---------|---------|
| CMake | 3.20+ | Build system |
| Git | 2.x+ | Dependency fetching |

C++ dependencies (SFML, EnTT, nlohmann/json, Google Test) are fetched automatically by CMake. You do not need to install them manually.

### Windows

- **Visual Studio 2019+** (with "Desktop development with C++" workload)
- Or **MSYS2/MinGW** with GCC 11+

```powershell
# Verify CMake is installed
cmake --version
```

### Linux (Ubuntu/Debian)

```bash
# Build tools
sudo apt update
sudo apt install build-essential cmake git

# SFML system dependencies (X11, OpenGL, audio)
sudo apt install libx11-dev libxrandr-dev libxcursor-dev \
    libxi-dev libudev-dev libgl1-mesa-dev \
    libopenal-dev libvorbis-dev libflac-dev \
    libfreetype-dev
```

### Linux (Fedora)

```bash
sudo dnf install cmake gcc-c++ git
sudo dnf install libX11-devel libXrandr-devel libXcursor-devel \
    libXi-devel systemd-devel mesa-libGL-devel \
    openal-soft-devel libvorbis-devel flac-devel \
    freetype-devel
```

### macOS

```bash
# Install Xcode command line tools
xcode-select --install

# Install CMake via Homebrew
brew install cmake
```

### WASM (Emscripten)

```bash
# Install Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest

# Activate in current shell (must be done every new terminal)
source ./emsdk_env.sh

# Verify
emcc --version
```

---

## Native Builds

### Debug Build (with sanitizers)

```bash
# Configure
cmake --preset debug-native

# Build
cmake --build build/debug-native

# Run
./build/debug-native/MarioGame        # Linux/macOS
.\build\debug-native\Debug\MarioGame.exe  # Windows (MSVC)
```

Debug builds enable:
- Address Sanitizer (ASan) + Undefined Behavior Sanitizer (UBSan) on GCC/Clang
- Full compiler warnings (`-Wall -Wextra -Wpedantic -Werror`)
- Debug symbols

### Release Build

```bash
# Configure
cmake --preset release-native

# Build
cmake --build build/release-native

# Run
./build/release-native/MarioGame      # Linux/macOS
.\build\release-native\Release\MarioGame.exe  # Windows (MSVC)
```

Release builds enable:
- Compiler optimizations (`-O2` / `/O2`)
- No sanitizers
- No debug symbols

---

## WASM Builds

Requires Emscripten SDK activated in the current shell.

### Debug WASM

```bash
source /path/to/emsdk/emsdk_env.sh

cmake --preset debug-wasm
cmake --build build/debug-wasm
```

Debug WASM enables `SAFE_HEAP=1` and `ASSERTIONS=1` for catching memory errors.

### Release WASM

```bash
source /path/to/emsdk/emsdk_env.sh

cmake --preset release-wasm
cmake --build build/release-wasm
```

Release WASM enables `-O2` optimization and Closure Compiler for smaller output.

### Testing WASM Locally

```bash
cd build/release-wasm

# Serve with any HTTP server (WASM requires HTTP, not file://)
python3 -m http.server 8080

# Open in browser
# http://localhost:8080/index.html
```

---

## Running Tests

Tests are only built for native builds (not WASM).

```bash
# Build with tests enabled (default)
cmake --preset debug-native
cmake --build build/debug-native

# Run all tests
cd build/debug-native
ctest --output-on-failure

# Run a specific test suite
./test_physics
./test_collision
./test_statemachine
./test_levelloader
```

To disable tests:

```bash
cmake --preset debug-native -DMARIO_BUILD_TESTS=OFF
```

---

## Build Presets Summary

| Preset | Platform | Build Type | Sanitizers | Tests |
|--------|----------|------------|------------|-------|
| `debug-native` | Desktop | Debug | ASan + UBSan | Yes |
| `release-native` | Desktop | Release | No | Yes |
| `debug-wasm` | Browser | Debug | SAFE_HEAP | No |
| `release-wasm` | Browser | Release | No | No |

---

## Common Errors and Fixes

### "SFML not found"

CMake will auto-fetch SFML via FetchContent if it's not installed system-wide. If fetching fails, check your internet connection. On Linux, ensure the X11/OpenGL development headers listed above are installed.

### "emcc not found" (WASM builds)

You need to activate the Emscripten SDK in your current shell:

```bash
source /path/to/emsdk/emsdk_env.sh
```

This must be done every time you open a new terminal.

### "Cannot find assets" at runtime

The build copies `assets/` to the binary output directory automatically. If running from a different directory, either:
- Run from the build directory: `cd build/debug-native && ./MarioGame`
- Or copy `assets/` next to the executable

### Tests fail with "assets directory not found"

Level loader tests need access to `assets/levels/level_01.json`. Run tests from the build directory where assets are copied:

```bash
cd build/debug-native
ctest --output-on-failure
```

### MSVC linker errors about SFML

Ensure you're using the correct CMake generator:

```powershell
cmake --preset debug-native -G "Visual Studio 17 2022"
```

### WASM file too large in debug mode

Debug WASM builds include assertions and safe-heap checks. Use `release-wasm` for production:

```bash
cmake --preset release-wasm
cmake --build build/release-wasm
```

### "Access denied" on Windows

Run your terminal as Administrator, or ensure your antivirus isn't blocking the build tools.
