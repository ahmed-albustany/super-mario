#pragma once

#include "scenes/IScene.hpp"
#include "core/GameState.hpp"
#include <string>
#include <functional>

class Game;

/// @brief "Level Complete!" overlay shown after reaching the trophy.
///        Displays score, then auto-advances to the next level or victory.
class LevelCompleteScene final : public IScene {
public:
    LevelCompleteScene(Game& game, GameStatePtr state, std::function<void()> onContinue);

    void onEnter() override;
    void onExit() override;
    void handleInput(const InputManager& input) override;
    void update(float dt) override;
    void render(IPlatform& platform) override;
    [[nodiscard]] bool isTransparent() const override { return true; }
    [[nodiscard]] std::string name() const override { return "LevelCompleteScene"; }

private:
    Game& m_game;
    GameStatePtr m_state;
    std::function<void()> m_onContinue;
    float m_elapsed = 0.0f;
    bool m_continued = false;

    static constexpr float AUTO_ADVANCE_DELAY = 3.0f;
};
