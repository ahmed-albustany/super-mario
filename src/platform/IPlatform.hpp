#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include "utils/Math.hpp"

// =============================================================================
// Lightweight types used across the platform boundary
// =============================================================================

/// @brief RGBA color.
struct Color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    constexpr Color() = default;
    constexpr Color(uint8_t pr, uint8_t pg, uint8_t pb, uint8_t pa = 255)
        : r(pr), g(pg), b(pb), a(pa) {}

    static constexpr Color White()       { return {255, 255, 255, 255}; }
    static constexpr Color Black()       { return {0,   0,   0,   255}; }
    static constexpr Color Transparent() { return {0,   0,   0,   0};   }
    static constexpr Color Red()         { return {255, 0,   0,   255}; }
    static constexpr Color Green()       { return {0,   255, 0,   255}; }
    static constexpr Color Blue()        { return {0,   0,   255, 255}; }
    static constexpr Color Yellow()      { return {255, 255, 0,   255}; }
    static constexpr Color Cyan()        { return {0,   255, 255, 255}; }
};

/// @brief Opaque handle to a loaded texture. id == 0 is invalid.
struct TextureHandle {
    uint32_t id = 0;
    [[nodiscard]] bool valid() const { return id != 0; }
};

/// @brief Opaque handle to a loaded sound effect. id == 0 is invalid.
struct SoundHandle {
    uint32_t id = 0;
    [[nodiscard]] bool valid() const { return id != 0; }
};

/// @brief Opaque handle to a loaded font. id == 0 is invalid.
struct FontHandle {
    uint32_t id = 0;
    [[nodiscard]] bool valid() const { return id != 0; }
};

/// @brief Abstract key codes — game logic uses these, never raw platform keys.
enum class KeyCode {
    Up, Down, Left, Right,
    Jump,    // Z, Space
    Run,     // X, LShift (run / fire)
    Pause,   // Escape, P
    Confirm, // Enter, Z
    Back,    // Escape, X
    Debug,   // F1

    // Player 2 keys (WASD + J/K)
    P2Up, P2Down, P2Left, P2Right,
    P2Jump,  // J
    P2Run,   // K

    // Player 3 keys (IJKL + N/M)
    P3Up, P3Down, P3Left, P3Right,
    P3Jump,  // N
    P3Run,   // M

    // Player 4 keys (Numpad 8456 + Numpad1/Numpad2)
    P4Up, P4Down, P4Left, P4Right,
    P4Jump,  // Numpad1
    P4Run,   // Numpad2

    COUNT    // sentinel — must be last
};

// =============================================================================
// Platform interface — the ONLY layer that touches SFML / SDL
// =============================================================================

/// @brief Pure virtual platform interface.
///        All rendering, input, audio, and window management go through here.
///        Game logic NEVER includes SFML or SDL headers — only this file.
class IPlatform {
public:
    virtual ~IPlatform() = default;

    // ---- Window lifecycle ----
    virtual void pollEvents() = 0;
    [[nodiscard]] virtual bool isRunning() const = 0;
    virtual void close() = 0;

    // ---- Rendering ----
    virtual void clear(const Color& color = Color::Black()) = 0;
    virtual void display() = 0;

    /// @brief Draw a sprite (sub-rect of a texture) at a position.
    virtual void drawSprite(TextureHandle texture,
                            const Rect& srcRect,
                            const Vec2f& position,
                            const Vec2f& scale = {1.0f, 1.0f},
                            float rotation = 0.0f,
                            bool flipX = false,
                            const Color& tint = Color::White()) = 0;

    /// @brief Draw a filled/outlined rectangle (for debug overlays and UI).
    virtual void drawRect(const Rect& rect,
                          const Color& fillColor,
                          const Color& outlineColor = Color::Transparent(),
                          float outlineThickness = 0.0f) = 0;

    /// @brief Draw text at a position.
    virtual void drawText(FontHandle font,
                          const std::string& text,
                          const Vec2f& position,
                          unsigned int size,
                          const Color& color = Color::White()) = 0;

    // ---- Timing ----
    [[nodiscard]] virtual float getDeltaTime() const = 0;

    // ---- Window info ----
    [[nodiscard]] virtual Vec2i getWindowSize() const = 0;

    // ---- Input ----
    [[nodiscard]] virtual bool isKeyPressed(KeyCode key) const = 0;
    [[nodiscard]] virtual bool isKeyJustPressed(KeyCode key) const = 0;
    [[nodiscard]] virtual bool isKeyJustReleased(KeyCode key) const = 0;
    [[nodiscard]] virtual Vec2f getTouchOrMousePosition() const = 0;
    [[nodiscard]] virtual bool isTouchOrMouseDown() const = 0;

    // ---- Audio ----
    virtual void playSound(SoundHandle sound) = 0;
    virtual void playMusic(const std::string& path, bool loop = true) = 0;
    virtual void stopMusic() = 0;
    virtual void setMusicVolume(float volume) = 0;
    virtual void setSfxVolume(float volume) = 0;

    // ---- Resource loading (returns handles) ----
    [[nodiscard]] virtual TextureHandle loadTexture(const std::string& key,
                                                     const std::string& path) = 0;
    [[nodiscard]] virtual SoundHandle loadSound(const std::string& key,
                                                 const std::string& path) = 0;
    [[nodiscard]] virtual FontHandle loadFont(const std::string& key,
                                               const std::string& path) = 0;

    // ---- Resource unloading ----
    virtual void unloadTexture(TextureHandle handle) = 0;
    virtual void unloadSound(SoundHandle handle) = 0;
    virtual void unloadFont(FontHandle handle) = 0;

    // ---- Camera transform (offset all drawing) ----
    virtual void setCameraOffset(const Vec2f& offset) = 0;
    [[nodiscard]] virtual Vec2f getCameraOffset() const = 0;
};
