#pragma once

#include "platform/IPlatform.hpp"

#ifdef MARIO_WASM

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#include <unordered_map>
#include <array>
#include <memory>
#include <string>

/// @brief SDL2 implementation of IPlatform — used for WASM/Emscripten builds.
///        Uses SDL2 + SDL_image + SDL_mixer + SDL_ttf, all available as
///        Emscripten ports (-s USE_SDL=2 etc.).
class SDLPlatform final : public IPlatform {
public:
    SDLPlatform();
    ~SDLPlatform() override;

    // Non-copyable
    SDLPlatform(const SDLPlatform&) = delete;
    SDLPlatform& operator=(const SDLPlatform&) = delete;

    // ---- Window lifecycle ----
    void pollEvents() override;
    [[nodiscard]] bool isRunning() const override;
    void close() override;

    // ---- Rendering ----
    void clear(const Color& color) override;
    void display() override;
    void drawSprite(TextureHandle texture, const Rect& srcRect, const Vec2f& position,
                    const Vec2f& scale, float rotation, bool flipX,
                    const Color& tint) override;
    void drawRect(const Rect& rect, const Color& fillColor,
                  const Color& outlineColor, float outlineThickness) override;
    void drawText(FontHandle font, const std::string& text, const Vec2f& position,
                  unsigned int size, const Color& color) override;

    // ---- Timing ----
    [[nodiscard]] float getDeltaTime() const override;

    // ---- Window info ----
    [[nodiscard]] Vec2i getWindowSize() const override;

    // ---- Input ----
    [[nodiscard]] bool isKeyPressed(KeyCode key) const override;
    [[nodiscard]] bool isKeyJustPressed(KeyCode key) const override;
    [[nodiscard]] bool isKeyJustReleased(KeyCode key) const override;
    [[nodiscard]] Vec2f getTouchOrMousePosition() const override;
    [[nodiscard]] bool isTouchOrMouseDown() const override;

    // ---- Audio ----
    void playSound(SoundHandle sound) override;
    void playMusic(const std::string& path, bool loop) override;
    void stopMusic() override;
    void setMusicVolume(float volume) override;
    void setSfxVolume(float volume) override;

    // ---- Resources ----
    [[nodiscard]] TextureHandle loadTexture(const std::string& key,
                                             const std::string& path) override;
    [[nodiscard]] SoundHandle loadSound(const std::string& key,
                                         const std::string& path) override;
    [[nodiscard]] FontHandle loadFont(const std::string& key,
                                       const std::string& path) override;
    void unloadTexture(TextureHandle handle) override;
    void unloadSound(SoundHandle handle) override;
    void unloadFont(FontHandle handle) override;

    // ---- Camera ----
    void setCameraOffset(const Vec2f& offset) override;
    [[nodiscard]] Vec2f getCameraOffset() const override;

private:
    static constexpr int KEY_COUNT = static_cast<int>(KeyCode::COUNT);

    void initKeyMap();

    // SDL core
    SDL_Window*   m_window   = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    bool m_running = true;
    Vec2f m_cameraOffset;

    // Timing
    Uint64 m_lastTime   = 0;
    Uint64 m_perfFreq   = 1;
    float  m_deltaTime  = 0.0f;

    // Input
    struct KeyBinding {
        SDL_Scancode primary   = SDL_SCANCODE_UNKNOWN;
        SDL_Scancode secondary = SDL_SCANCODE_UNKNOWN;
    };
    std::array<KeyBinding, KEY_COUNT> m_keyMap{};
    std::array<bool, KEY_COUNT> m_currKeys{};
    std::array<bool, KEY_COUNT> m_prevKeys{};

    // Touch / mouse
    Vec2f m_touchPos;
    bool  m_touchDown = false;

    // Textures (SDL_Texture with custom deleter)
    struct TextureData {
        SDL_Texture* texture = nullptr;
        int width  = 0;
        int height = 0;
    };
    std::unordered_map<uint32_t, TextureData> m_textures;
    uint32_t m_nextTexId = 1;

    // Sound effects (Mix_Chunk*)
    std::unordered_map<uint32_t, Mix_Chunk*> m_sounds;
    uint32_t m_nextSndId = 1;
    int m_sfxVolume = MIX_MAX_VOLUME;

    // Fonts (keyed by id → path, since SDL_ttf needs size at open time)
    struct FontData {
        std::string path;
        std::unordered_map<unsigned int, TTF_Font*> sizedFonts; // cache by size
    };
    std::unordered_map<uint32_t, FontData> m_fonts;
    uint32_t m_nextFontId = 1;

    // Music (only one at a time)
    Mix_Music* m_music = nullptr;

    TTF_Font* getOrOpenFont(uint32_t fontId, unsigned int size);
};

/// @brief Factory function (called by PlatformFactory via forward declaration).
std::unique_ptr<IPlatform> createSDLPlatform();

#endif // MARIO_WASM
