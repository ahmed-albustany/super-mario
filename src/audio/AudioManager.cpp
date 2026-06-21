#include "audio/AudioManager.hpp"
#include "core/Events.hpp"
#include "core/ResourceManager.hpp"
#include "core/GameConfig.hpp"
#include "utils/Logger.hpp"
#include "utils/Math.hpp"

// =============================================================================
// Init / Shutdown
// =============================================================================

void AudioManager::init(IPlatform& platform, EventBus& eventBus) {
    m_platform = &platform;

    m_masterVolume    = 1.0f;
    m_sfxVolume       = Config::DEFAULT_SFX_VOL;
    m_musicVolume     = Config::DEFAULT_MUSIC_VOL;
    m_fading          = false;
    m_fadeDuration    = 0.0f;
    m_fadeTimer       = 0.0f;
    m_fadeStartVolume = 0.0f;
    m_currentMusicKey.clear();

    applyMusicVolume();
    m_platform->setSfxVolume(m_masterVolume * m_sfxVolume);

    // Subscribe to gameplay events
    m_subLevelComplete = eventBus.subscribe<LevelCompleteEvent>(
        [this](const LevelCompleteEvent& /*e*/) {
            playSound("level_complete");
        });

    m_subPlayerDied = eventBus.subscribe<PlayerDiedEvent>(
        [this](const PlayerDiedEvent& /*e*/) {
            playSound("player_death");
        });

    m_subCoinCollected = eventBus.subscribe<CoinCollectedEvent>(
        [this](const CoinCollectedEvent& /*e*/) {
            playSound("coin_pickup");
        });

    m_subEnemyKilled = eventBus.subscribe<EnemyKilledEvent>(
        [this](const EnemyKilledEvent& /*e*/) {
            playSound("enemy_stomp");
        });

    m_subPowerUp = eventBus.subscribe<PowerUpActivatedEvent>(
        [this](const PowerUpActivatedEvent& /*e*/) {
            playSound("powerup_pickup");
        });

    m_subPlayerJump = eventBus.subscribe<PlayerJumpEvent>(
        [this](const PlayerJumpEvent& /*e*/) {
            playSound("jump");
        });

    m_subPlayerLanded = eventBus.subscribe<PlayerLandedEvent>(
        [this](const PlayerLandedEvent& /*e*/) {
            playSound("land");
        });

    m_subPlayerHurt = eventBus.subscribe<PlayerHurtEvent>(
        [this](const PlayerHurtEvent& /*e*/) {
            playSound("player_hurt");
        });

    m_subEnemyShoot = eventBus.subscribe<EnemyShootEvent>(
        [this](const EnemyShootEvent& /*e*/) {
            playSound("enemy_shoot");
        });

    m_subPowerUpExpired = eventBus.subscribe<PowerUpExpiredEvent>(
        [this](const PowerUpExpiredEvent& /*e*/) {
            playSound("powerup_expire");
        });

    m_subFireball = eventBus.subscribe<FireballEvent>(
        [this](const FireballEvent& /*e*/) {
            playSound("fireball");
        });

    m_subBlockHit = eventBus.subscribe<BlockHitEvent>(
        [this](const BlockHitEvent& /*e*/) {
            playSound("block_hit");
        });

    LOG_INFO("AudioManager: initialized");
}

void AudioManager::shutdown(EventBus& eventBus) {
    eventBus.unsubscribe<LevelCompleteEvent>(m_subLevelComplete);
    eventBus.unsubscribe<PlayerDiedEvent>(m_subPlayerDied);
    eventBus.unsubscribe<CoinCollectedEvent>(m_subCoinCollected);
    eventBus.unsubscribe<EnemyKilledEvent>(m_subEnemyKilled);
    eventBus.unsubscribe<PowerUpActivatedEvent>(m_subPowerUp);
    eventBus.unsubscribe<PlayerJumpEvent>(m_subPlayerJump);
    eventBus.unsubscribe<PlayerLandedEvent>(m_subPlayerLanded);
    eventBus.unsubscribe<PlayerHurtEvent>(m_subPlayerHurt);
    eventBus.unsubscribe<EnemyShootEvent>(m_subEnemyShoot);
    eventBus.unsubscribe<PowerUpExpiredEvent>(m_subPowerUpExpired);
    eventBus.unsubscribe<FireballEvent>(m_subFireball);
    eventBus.unsubscribe<BlockHitEvent>(m_subBlockHit);

    m_platform = nullptr;
    LOG_INFO("AudioManager: shut down");
}

// =============================================================================
// Update (fade tick)
// =============================================================================

void AudioManager::update(float dt) {
    if (!m_fading || !m_platform) return;

    m_fadeTimer += dt;
    if (m_fadeTimer >= m_fadeDuration) {
        m_fading = false;
        m_platform->stopMusic();
        m_currentMusicKey.clear();
        applyMusicVolume();
    } else {
        float t = m_fadeTimer / m_fadeDuration;
        float fadeVolume = m_fadeStartVolume * (1.0f - t);
        m_platform->setMusicVolume(fadeVolume);
    }
}

// =============================================================================
// Sound effects
// =============================================================================

void AudioManager::playSound(const std::string& key) {
    if (!m_platform) return;

    auto handle = ResourceManager::instance().getSound(key);
    if (handle) {
        m_platform->setSfxVolume(m_masterVolume * m_sfxVolume);
        m_platform->playSound(*handle);
    } else {
        LOG_WARN("AudioManager: sound '" << key << "' not found — skipping");
    }
}

// =============================================================================
// Music
// =============================================================================

void AudioManager::playMusic(const std::string& key, bool loop) {
    if (!m_platform) return;

    if (key == m_currentMusicKey && !m_fading) return;

    m_fading = false;
    m_platform->stopMusic();

#ifdef MARIO_WASM
    std::string path = "/assets/audio/" + key + ".ogg";
#else
    std::string path = "assets/audio/" + key + ".ogg";
#endif

    applyMusicVolume();
    m_platform->playMusic(path, loop);
    m_currentMusicKey = key;

    LOG_INFO("AudioManager: playing music '" << key << "'");
}

void AudioManager::stopMusic() {
    if (!m_platform) return;

    m_fading = false;
    m_platform->stopMusic();
    m_currentMusicKey.clear();
}

void AudioManager::fadeOutMusic(float duration) {
    if (!m_platform || m_currentMusicKey.empty()) return;

    if (duration <= 0.0f) {
        stopMusic();
        return;
    }

    m_fading          = true;
    m_fadeDuration    = duration;
    m_fadeTimer       = 0.0f;
    m_fadeStartVolume = m_masterVolume * m_musicVolume;
}

// =============================================================================
// Volume
// =============================================================================

void AudioManager::setMasterVolume(float volume) {
    m_masterVolume = Math::clamp(volume, 0.0f, 1.0f);
    applyMusicVolume();
    if (m_platform) {
        m_platform->setSfxVolume(m_masterVolume * m_sfxVolume);
    }
}

void AudioManager::setSFXVolume(float volume) {
    m_sfxVolume = Math::clamp(volume, 0.0f, 1.0f);
    if (m_platform) {
        m_platform->setSfxVolume(m_masterVolume * m_sfxVolume);
    }
}

void AudioManager::setMusicVolume(float volume) {
    m_musicVolume = Math::clamp(volume, 0.0f, 1.0f);
    applyMusicVolume();
}

void AudioManager::applyMusicVolume() {
    if (!m_platform) return;
    if (!m_fading) {
        m_platform->setMusicVolume(m_masterVolume * m_musicVolume);
    }
}
