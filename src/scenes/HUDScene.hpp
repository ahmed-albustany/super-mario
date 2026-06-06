#pragma once

#include "scenes/IScene.hpp"
#include "core/EventBus.hpp"
#include "core/GameState.hpp"
#include <string>

class Game;

/// @brief Transparent overlay — draws Mario-style HUD on top of GameScene.
///        Uses shared_ptr to GameState, so no dangling references if GameScene dies.
class HUDScene final : public IScene {
public:
    HUDScene(Game& game, GameStatePtr state);

    void onEnter() override;
    void onExit() override;
    void handleInput(const InputManager& input) override;
    void update(float dt) override;
    void render(IPlatform& platform) override;
    [[nodiscard]] bool isTransparent() const override { return true; }
    [[nodiscard]] bool passesUpdate() const override { return true; }
    [[nodiscard]] std::string name() const override { return "HUDScene"; }

private:
    Game& m_game;
    GameStatePtr m_state;

    // Star power display
    float m_powerUpTimer    = 0.0f;
    float m_powerUpDuration = 0.0f;
    bool  m_powerUpActive   = false;
    SubscriberID m_subPowerUp = 0;
};
