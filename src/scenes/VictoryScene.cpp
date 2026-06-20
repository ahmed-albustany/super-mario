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

    platform.drawText(f, "All levels complete!",
                      {screenW * 0.5f - 120.0f, titleY + 60.0f}, 18,
                      Color{255, 255, 255, 220});

    // Score summary
    float sumY = 280.0f;
    if (m_state) {
        static const char* CHAR_NAMES[] = {"MASK DUDE", "NINJA FROG", "PINK MAN", "VIRTUAL GUY"};
        static const Color CHAR_COLORS[] = {
            {78, 205, 196, 255}, {255, 107, 107, 255},
            {255, 182, 193, 255}, {130, 130, 255, 255}
        };

        int numActive = m_state->numActivePlayers();
        float colWidth = 160.0f;
        float startX = screenW * 0.5f - (static_cast<float>(numActive) * colWidth) * 0.5f;

        int totalScore = 0;
        for (int i = 0; i < numActive; ++i) {
            float colX = startX + static_cast<float>(i) * colWidth;
            const auto& ps = m_state->players[static_cast<size_t>(i)];
            totalScore += ps.score;

            platform.drawText(f, CHAR_NAMES[static_cast<size_t>(i)],
                              {colX, sumY}, 14,
                              CHAR_COLORS[static_cast<size_t>(i)]);
            platform.drawText(f, std::to_string(ps.score),
                              {colX, sumY + 20.0f}, 20,
                              Color{255, 255, 255, 255});
        }

        platform.drawText(f, "TOTAL SCORE",
                          {screenW * 0.5f - 80.0f, sumY + 60.0f}, 16,
                          Color{255, 220, 50, 200});
        platform.drawText(f, std::to_string(totalScore),
                          {screenW * 0.5f - 60.0f, sumY + 86.0f}, 32,
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
