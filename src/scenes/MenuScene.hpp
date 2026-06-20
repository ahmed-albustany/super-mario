#pragma once

#include "scenes/IScene.hpp"
#include <string>

class Game;

/// @brief Main menu — PIXEL RUSH with mode selection.
///        Solo, 2P Alternating, 2P Co-op, 4P Co-op, 4P VS, Quit.
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
    float m_titleBob     = 0.0f;
    float m_elapsed      = 0.0f;
    bool  m_confirmed    = false;

    static constexpr int MENU_SOLO      = 0;
    static constexpr int MENU_2P_ALT    = 1;
    static constexpr int MENU_2P_COOP   = 2;
    static constexpr int MENU_4P_COOP   = 3;
    static constexpr int MENU_4P_VS     = 4;
    static constexpr int MENU_QUIT      = 5;
    static constexpr int MENU_COUNT     = 6;

    void selectItem(int index);
    void confirmSelection();
};
