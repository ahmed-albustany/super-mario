#pragma once

#include "scenes/IScene.hpp"
#include "core/EventBus.hpp"
#include "core/GameState.hpp"
#include <string>

class Game;

/// @brief Transparent overlay — draws PIXEL RUSH HUD on top of GameScene.
///        Supports 1-4 player display based on GameMode.
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
    GameStatePtr m_state;
};
