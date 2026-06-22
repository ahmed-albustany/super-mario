#pragma once

#include "scenes/IScene.hpp"
#include "core/GameState.hpp"
#include <string>

class Game;

/// @brief Overlay pause menu — drawn on top of GameScene.
///        Resume / Restart Level / Quit to Menu. Dims the background.
class PauseScene final : public IScene {
public:
    PauseScene(Game& game, GameMode mode = GameMode::Solo, int currentLevel = 0);

    void onEnter() override;
    void onExit() override;
    void handleInput(const InputManager& input) override;
    void update(float dt) override;
    void render(IPlatform& platform) override;
    [[nodiscard]] bool isTransparent() const override { return true; }
    [[nodiscard]] std::string name() const override { return "PauseScene"; }

private:
    Game& m_game;
    GameMode m_restartMode  = GameMode::Solo;
    int   m_restartLevel    = 0;
    int   m_selectedItem    = 0;
    float m_elapsed         = 0.0f;

    static constexpr int PAUSE_RESUME  = 0;
    static constexpr int PAUSE_RESTART = 1;
    static constexpr int PAUSE_QUIT    = 2;
    static constexpr int PAUSE_COUNT   = 3;

    void selectItem(int index);
    void confirmSelection();
};
