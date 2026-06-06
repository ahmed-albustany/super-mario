#pragma once

#include "scenes/IScene.hpp"
#include "core/GameState.hpp"
#include <string>

class Game;

/// @brief "Congratulations!" screen shown after completing all levels.
///        Displays final score summary and options to play again or return to menu.
class VictoryScene final : public IScene {
public:
    VictoryScene(Game& game, GameStatePtr state);

    void onEnter() override;
    void onExit() override;
    void handleInput(const InputManager& input) override;
    void update(float dt) override;
    void render(IPlatform& platform) override;
    [[nodiscard]] bool isTransparent() const override { return true; }
    [[nodiscard]] std::string name() const override { return "VictoryScene"; }

private:
    Game& m_game;
    GameStatePtr m_state;
    int   m_selectedItem = 0;
    float m_elapsed = 0.0f;

    static constexpr int V_MENU  = 0;
    static constexpr int V_COUNT = 1;

    void confirmSelection();
};
