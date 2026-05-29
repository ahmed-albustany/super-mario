#pragma once

#include <memory>
#include "platform/IPlatform.hpp"
#include "core/SceneManager.hpp"
#include "core/InputManager.hpp"
#include "core/EventBus.hpp"
#include "audio/AudioManager.hpp"

/// @brief Main game class — owns all engine subsystems and runs the game loop.
///        Constructed with a platform; call tick() once per frame.
///        Uses a fixed-timestep accumulator pattern for deterministic physics.
class Game {
public:
    /// @brief Construct the game with a platform implementation.
    explicit Game(std::unique_ptr<IPlatform> platform);
    ~Game();

    // Non-copyable
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    /// @brief Run one frame: poll → input → fixed update(s) → render.
    void tick();

    /// @brief Is the game still running?
    [[nodiscard]] bool isRunning() const;

    // ---- Subsystem access (for scenes) ----
    [[nodiscard]] IPlatform&      platform()     { return *m_platform; }
    [[nodiscard]] SceneManager&   scenes()       { return m_sceneManager; }
    [[nodiscard]] InputManager&   input()        { return m_inputManager; }
    [[nodiscard]] EventBus&       events()       { return m_eventBus; }
    [[nodiscard]] AudioManager&   audio()        { return AudioManager::instance(); }

private:
    void initSubsystems();

    std::unique_ptr<IPlatform> m_platform;
    SceneManager               m_sceneManager;
    InputManager               m_inputManager;
    EventBus                   m_eventBus;

    float m_accumulator = 0.0f;
};
