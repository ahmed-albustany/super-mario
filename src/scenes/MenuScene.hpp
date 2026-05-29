#pragma once

#include "scenes/IScene.hpp"
#include <string>

class Game;

/// @brief Main menu — animated title, Play / Quit buttons.
///        Navigable via keyboard (Up/Down + Confirm) and mouse/touch.
class MenuScene final : public IScene {
public:
    explicit MenuScene(Game& game);

    void onEnter() override;
    void onExit() override;
    void handleInput(const InputManager& input) override;
    void update(float dt) override;
    void render(IPlatform& platform) override;
    [[nodiscard]] std::string name() const override { return "MenuScene"; }

private:
    Game& m_game;
    int   m_selectedItem = 0;
    float m_titleBob     = 0.0f; // animated y offset for title
    float m_elapsed      = 0.0f;

    static constexpr int MENU_PLAY = 0;
    static constexpr int MENU_QUIT = 1;
    static constexpr int MENU_COUNT = 2;

    void selectItem(int index);
    void confirmSelection();
};
