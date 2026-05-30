#pragma once

#include "platform/IPlatform.hpp"
#include "core/EventBus.hpp"
#include <string>

/// @brief Singleton audio manager — wraps IPlatform audio calls.
///        Manages sound playback, music with fade, volume controls.
///        Session only — no volume persistence to disk.
///        Resets fully on init().
class AudioManager {
public:
    /// @brief Get the singleton instance.
    static AudioManager& instance() {
        static AudioManager am;
        return am;
    }

    // Non-copyable
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    /// @brief Bind to a platform and event bus. Resets all state.
    void init(IPlatform& platform, EventBus& eventBus);

    /// @brief Unbind event subscriptions.
    void shutdown(EventBus& eventBus);

    /// @brief Tick fade timers. Called by Game::tick() each frame.
    void update(float dt);

    // ---- Sound effects ----

    /// @brief Play a sound effect by resource key. Max 16 concurrent via platform pool.
    void playSound(const std::string& key);

    // ---- Music ----

    /// @brief Play background music by resource key. Stops current music first.
    void playMusic(const std::string& key, bool loop = true);

    /// @brief Stop music immediately.
    void stopMusic();

    /// @brief Fade out current music over duration seconds, then stop.
    void fadeOutMusic(float duration);

    // ---- Volume (0.0 – 1.0, session only) ----

    void setMasterVolume(float volume);
    void setSFXVolume(float volume);
    void setMusicVolume(float volume);

    [[nodiscard]] float getMasterVolume() const { return m_masterVolume; }
    [[nodiscard]] float getSFXVolume()   const { return m_sfxVolume; }
    [[nodiscard]] float getMusicVolume() const { return m_musicVolume; }

private:
    AudioManager() = default;

    /// @brief Apply effective music volume to platform (master * music * fade).
    void applyMusicVolume();

    IPlatform* m_platform = nullptr;

    // Volume controls (session only — never written to disk)
    float m_masterVolume = 1.0f;
    float m_sfxVolume    = 1.0f;
    float m_musicVolume  = 0.75f;

    // Fade state
    bool  m_fading         = false;
    float m_fadeDuration   = 0.0f;
    float m_fadeTimer      = 0.0f;
    float m_fadeStartVolume = 0.0f;

    // Current music key (to avoid restarting same track)
    std::string m_currentMusicKey;

    // Event subscriptions
    SubscriberID m_subLevelComplete = 0;
    SubscriberID m_subPlayerDied   = 0;
    SubscriberID m_subCoinCollected = 0;
    SubscriberID m_subEnemyKilled  = 0;
    SubscriberID m_subPowerUp      = 0;
    SubscriberID m_subPlayerJump   = 0;
    SubscriberID m_subPlayerDash   = 0;
    SubscriberID m_subPlayerWallJump = 0;
    SubscriberID m_subPlayerLanded = 0;
    SubscriberID m_subPlayerHurt   = 0;
    SubscriberID m_subEnemyShoot   = 0;
    SubscriberID m_subPowerUpExpired = 0;
};
