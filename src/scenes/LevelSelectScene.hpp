#pragma once

#include "scenes/IScene.hpp"
#include "core/GameState.hpp"
#include <string>

class Game;

/// @brief Level selection screen — pick from 10 levels before starting gameplay.
class LevelSelectScene final : public IScene {
public:
    LevelSelectScene(Game& game, GameMode mode);

    void onEnter() override;
    void onExit() override;
    void handleInput(const InputManager& input) override;
    void update(float dt) override;
    void render(IPlatform& platform) override;
    [[nodiscard]] std::string name() const override { return "LevelSelectScene"; }

private:
    Game&    m_game;
    GameMode m_mode;
    int      m_selectedLevel = 0;
    float    m_elapsed       = 0.0f;
    bool     m_confirmed     = false;

    void selectLevel(int index);
    void confirmSelection();
};
