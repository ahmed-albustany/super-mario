#include "scenes/VictoryScene.hpp"
#include "scenes/MenuScene.hpp"
#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/InputManager.hpp"
#include "core/ResourceManager.hpp"
#include "audio/AudioManager.hpp"

#include <cmath>
#include <string>

VictoryScene::VictoryScene(Game& game, GameStatePtr state)
    : m_game(game)
    , m_state(std::move(state))
{}

void VictoryScene::onEnter() {
    m_selectedItem = V_MENU;
    m_elapsed = 0.0f;
    AudioManager::instance().stopMusic();
    AudioManager::instance().playSound("level_complete");
}

void VictoryScene::onExit() {}

void VictoryScene::handleInput(const InputManager& input) {
    if (m_elapsed < 1.0f) return;

    if (input.isJustPressed(Action::Confirm) || input.isJustPressed(Action::Jump)) {
        confirmSelection();
    }
}

void VictoryScene::update(float dt) {
    m_elapsed += dt;
}

void VictoryScene::render(IPlatform& platform) {
    float screenW = static_cast<float>(Config::WINDOW_WIDTH);
    float screenH = static_cast<float>(Config::WINDOW_HEIGHT);

    // Dim overlay
    platform.drawRect({0, 0, screenW, screenH}, Color{0, 0, 0, 200});

    auto font = ResourceManager::instance().getFont("main");
    if (!font) return;
    FontHandle f = *font;

    // Title
    float titleY = 100.0f + std::sin(m_elapsed * 1.5f) * 8.0f;
    platform.drawText(f, "CONGRATULATIONS!",
                      {screenW * 0.5f - 180.0f, titleY}, 36,
                      Color{255, 220, 50, 255});

    platform.drawText(f, "You defeated Bowser!",
                      {screenW * 0.5f - 120.0f, titleY + 60.0f}, 18,
                      Color{255, 255, 255, 220});

    // Score summary
    float sumY = 280.0f;
    if (m_state) {
        platform.drawText(f, "MARIO",
                          {screenW * 0.5f - 160.0f, sumY}, 16,
                          Color{228, 0, 8, 255});
        platform.drawText(f, std::to_string(m_state->p1.score),
                          {screenW * 0.5f - 160.0f, sumY + 24.0f}, 22,
                          Color{255, 255, 255, 255});

        if (m_state->numPlayers == 2) {
            platform.drawText(f, "LUIGI",
                              {screenW * 0.5f + 40.0f, sumY}, 16,
                              Color{0, 148, 0, 255});
            platform.drawText(f, std::to_string(m_state->p2.score),
                              {screenW * 0.5f + 40.0f, sumY + 24.0f}, 22,
                              Color{255, 255, 255, 255});
        }

        int totalScore = m_state->p1.score + m_state->p2.score;
        platform.drawText(f, "TOTAL SCORE",
                          {screenW * 0.5f - 80.0f, sumY + 70.0f}, 16,
                          Color{255, 220, 50, 200});
        platform.drawText(f, std::to_string(totalScore),
                          {screenW * 0.5f - 60.0f, sumY + 96.0f}, 32,
                          Color{255, 220, 50, 255});
    }

    // Button
    float btnY = 480.0f;
    float btnW = 220.0f;
    float btnX = (screenW - btnW) * 0.5f;

    Color bg = Color{80, 60, 120, 220};
    Color border = Color{180, 140, 220, 255};
    platform.drawRect({btnX, btnY, btnW, 44.0f}, bg, border, 2.0f);

    platform.drawText(f, "Main Menu",
                      {btnX + 44.0f, btnY + 10.0f}, 20,
                      Color{255, 220, 150, 255});

    float bob = std::sin(m_elapsed * 4.0f) * 4.0f;
    platform.drawText(f, ">",
                      {btnX - 24.0f + bob, btnY + 10.0f}, 20,
                      Color{255, 220, 150, 255});

    // Hint
    if (m_elapsed >= 1.0f) {
        float alpha = static_cast<float>(std::fmod(m_elapsed, 2.0) > 1.0 ? 180 : 255);
        platform.drawText(f, "Press ENTER",
                          {screenW * 0.5f - 60.0f, 560.0f}, 12,
                          Color{255, 255, 255, static_cast<uint8_t>(alpha)});
    }
}

void VictoryScene::confirmSelection() {
    AudioManager::instance().playSound("menu_confirm");
    // Pop VictoryScene, pop HUD, replace GameScene with MenuScene
    m_game.scenes().pop();   // VictoryScene
    m_game.scenes().pop();   // HUDScene
    m_game.scenes().replace(std::make_unique<MenuScene>(m_game));
}
