#pragma once

#include "scenes/IScene.hpp"
#include <string>

class Game;

/// @brief Results screen — "GAME OVER" or "YOU WIN", final score,
///        Play Again / Main Menu buttons.
///        Score passed by value — pure session display, no persistence.
class GameOverScene final : public IScene {
public:
    /// @param win   true = level completed, false = all lives lost or time up.
    /// @param score Final score (copied by value).
    GameOverScene(Game& game, bool win, int score);

    void onEnter() override;
    void onExit() override;
    void handleInput(const InputManager& input) override;
    void update(float dt) override;
    void render(IPlatform& platform) override;
    [[nodiscard]] bool isTransparent() const override { return true; }
    [[nodiscard]] std::string name() const override { return "GameOverScene"; }

private:
    Game& m_game;
    bool  m_win;
    int   m_score;
    int   m_selectedItem = 0;
    float m_elapsed = 0.0f;

    static constexpr int GO_RETRY = 0;
    static constexpr int GO_MENU  = 1;
    static constexpr int GO_COUNT = 2;

    void selectItem(int index);
    void confirmSelection();
};
