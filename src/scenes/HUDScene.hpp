#pragma once

#include "scenes/IScene.hpp"
#include "core/EventBus.hpp"
#include <string>

class Game;

/// @brief Transparent overlay — draws score, lives, coins, gems, timer,
///        and power-up bar on top of GameScene every frame.
///        Reads state by reference from GameScene's counters.
class HUDScene final : public IScene {
public:
    /// @brief Constructed with references to GameScene's live counters.
    HUDScene(Game& game, const int& score, const int& lives,
             const int& coins, const int& gems, const float& timer);

    void onEnter() override;
    void onExit() override;
    void handleInput(const InputManager& input) override;
    void update(float dt) override;
    void render(IPlatform& platform) override;
    [[nodiscard]] bool isTransparent() const override { return true; }
    [[nodiscard]] std::string name() const override { return "HUDScene"; }

private:
    Game& m_game;

    // References to GameScene counters (RAM only, no copies)
    const int&   m_score;
    const int&   m_lives;
    const int&   m_coins;
    const int&   m_gems;
    const float& m_timer;

    // Power-up display state
    float m_powerUpTimer    = 0.0f;
    float m_powerUpDuration = 0.0f;
    bool  m_powerUpActive   = false;
    SubscriberID m_subPowerUp = 0;
};
