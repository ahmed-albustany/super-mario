#pragma once

#include "platform/IPlatform.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <unordered_map>
#include <array>
#include <memory>
#include <string>

/// @brief SFML 2.6 implementation of IPlatform — used for native desktop builds.
class SFMLPlatform final : public IPlatform {
public:
    SFMLPlatform();
    ~SFMLPlatform() override;

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

    // ---- Camera ----
    void setCameraOffset(const Vec2f& offset) override;
    [[nodiscard]] Vec2f getCameraOffset() const override;

private:
    static constexpr int KEY_COUNT = static_cast<int>(KeyCode::COUNT);

    void initKeyMap();
    static sf::Color toSFColor(const Color& c);

    sf::RenderWindow m_window;
    sf::Clock m_clock;
    float m_deltaTime = 0.0f;
    bool m_running = true;
    Vec2f m_cameraOffset;

    // Key state tracking: current frame vs previous frame
    std::array<bool, KEY_COUNT> m_currKeys{};
    std::array<bool, KEY_COUNT> m_prevKeys{};

    // Maps abstract KeyCode to one or more SFML keys
    struct KeyBinding {
        sf::Keyboard::Key primary   = sf::Keyboard::Unknown;
        sf::Keyboard::Key secondary = sf::Keyboard::Unknown;
    };
    std::array<KeyBinding, KEY_COUNT> m_keyMap{};

    // Mouse state
    Vec2f m_mousePos;
    bool m_mouseDown = false;

    // Resources — indexed by handle id (0 = invalid, never used)
    std::unordered_map<uint32_t, std::unique_ptr<sf::Texture>>     m_textures;
    std::unordered_map<uint32_t, std::unique_ptr<sf::SoundBuffer>> m_soundBuffers;
    std::unordered_map<uint32_t, std::unique_ptr<sf::Font>>        m_fonts;
    uint32_t m_nextTexId   = 1;
    uint32_t m_nextSndId   = 1;
    uint32_t m_nextFontId  = 1;

    // Sound pool — reuse sf::Sound instances (max 16 concurrent)
    static constexpr int SOUND_POOL_SIZE = 16;
    std::array<sf::Sound, SOUND_POOL_SIZE> m_soundPool;
    float m_sfxVolume = 100.0f;

    // Music — only one playing at a time
    sf::Music m_music;
};

/// @brief Factory function (called by PlatformFactory via forward declaration).
std::unique_ptr<IPlatform> createSFMLPlatform();
