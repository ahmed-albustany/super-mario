#pragma once

#include "scenes/IScene.hpp"
#include <string>

class Game;

/// @brief Overlay pause menu — drawn on top of GameScene.
///        Resume / Quit to Menu. Dims the background.
class PauseScene final : public IScene {
public:
    explicit PauseScene(Game& game);

    void onEnter() override;
    void onExit() override;
    void handleInput(const InputManager& input) override;
    void update(float dt) override;
    void render(IPlatform& platform) override;
    [[nodiscard]] bool isTransparent() const override { return true; }
    [[nodiscard]] std::string name() const override { return "PauseScene"; }

private:
    Game& m_game;
    int   m_selectedItem = 0;
    float m_elapsed = 0.0f;

    static constexpr int PAUSE_RESUME  = 0;
    static constexpr int PAUSE_QUIT    = 1;
    static constexpr int PAUSE_COUNT   = 2;

    void selectItem(int index);
    void confirmSelection();
};
