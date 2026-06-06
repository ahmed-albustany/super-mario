#pragma once

#include "scenes/IScene.hpp"
#include "core/GameState.hpp"
#include <string>

class Game;

/// @brief "Get Ready!" interstitial shown between player turns in alternating mode.
///        Displays for a brief duration then auto-pops, returning control to GameScene.
class GetReadyScene final : public IScene {
public:
    GetReadyScene(Game& game, GameStatePtr state);

    void onEnter() override;
    void onExit() override;
    void handleInput(const InputManager& input) override;
    void update(float dt) override;
    void render(IPlatform& platform) override;
    [[nodiscard]] bool isTransparent() const override { return true; }
    [[nodiscard]] std::string name() const override { return "GetReadyScene"; }

private:
    Game& m_game;
    GameStatePtr m_state;
    float m_timer = 0.0f;

    static constexpr float DISPLAY_DURATION = 2.0f;
};
