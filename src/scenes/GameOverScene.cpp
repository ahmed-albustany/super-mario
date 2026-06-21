#include "scenes/GameOverScene.hpp"
#include "scenes/MenuScene.hpp"
#include "scenes/GameScene.hpp"
#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/InputManager.hpp"
#include "core/ResourceManager.hpp"
#include "audio/AudioManager.hpp"

#include <cmath>
#include <string>

GameOverScene::GameOverScene(Game& game, bool win, int score, GameMode mode)
    : m_game(game)
    , m_win(win)
    , m_score(score)
    , m_mode(mode)
{}

void GameOverScene::onEnter() {
    m_selectedItem = GO_RETRY;
    m_elapsed = 0.0f;
    AudioManager::instance().playSound(m_win ? "level_complete" : "game_over");
}

void GameOverScene::onExit() {}

void GameOverScene::handleInput(const InputManager& input) {
    // Brief delay before accepting input (prevent accidental skip)
    if (m_elapsed < 0.5f) return;

    if (input.isJustPressed(Action::Confirm) || input.isJustPressed(Action::Jump)) {
        confirmSelection();
        return;
    }

    if (m_game.platform().isKeyJustPressed(KeyCode::Up)) {
        selectItem(m_selectedItem - 1);
    }
    if (m_game.platform().isKeyJustPressed(KeyCode::Down)) {
        selectItem(m_selectedItem + 1);
    }

    // Mouse/touch
    if (input.isPointerDown()) {
        Vec2f pos = input.getPointerPosition();
        float screenW = static_cast<float>(Config::WINDOW_WIDTH);
        float baseY = 400.0f;
        float itemH = 60.0f;
        float btnW = 220.0f;
        float btnX = (screenW - btnW) * 0.5f;

        for (int i = 0; i < GO_COUNT; ++i) {
            float btnY = baseY + static_cast<float>(i) * itemH;
            if (Rect{btnX, btnY, btnW, 44.0f}.contains(pos)) {
                m_selectedItem = i;
                confirmSelection();
                return;
            }
        }
    }
}

void GameOverScene::update(float dt) {
    m_elapsed += dt;
}

void GameOverScene::render(IPlatform& platform) {
    float screenW = static_cast<float>(Config::WINDOW_WIDTH);
    float screenH = static_cast<float>(Config::WINDOW_HEIGHT);

    // Dim overlay
    platform.drawRect({0, 0, screenW, screenH}, Color{0, 0, 0, 180});

    auto font = ResourceManager::instance().getFont("main");

    // Title
    if (font) {
        const char* title = m_win ? "YOU WIN!" : "GAME OVER";
        Color titleColor = m_win
            ? Color{100, 255, 150, 255}
            : Color{255, 80, 80, 255};

        float titleY = 200.0f + std::sin(m_elapsed * 1.5f) * 5.0f;
        platform.drawText(*font, title,
                          {screenW * 0.5f - 120.0f, titleY}, 48, titleColor);

        // Score
        platform.drawText(*font, "Final Score",
                          {screenW * 0.5f - 70.0f, 290.0f}, 18,
                          Color{180, 180, 200, 255});
        platform.drawText(*font, std::to_string(m_score),
                          {screenW * 0.5f - 50.0f, 320.0f}, 36,
                          Color{255, 220, 100, 255});
    }

    // Buttons
    const char* labels[GO_COUNT] = {"Play Again", "Main Menu"};
    float baseY = 400.0f;
    float itemH = 60.0f;
    float btnW = 220.0f;
    float btnX = (screenW - btnW) * 0.5f;

    for (int i = 0; i < GO_COUNT; ++i) {
        float btnY = baseY + static_cast<float>(i) * itemH;
        bool selected = (i == m_selectedItem);

        Color bg     = selected ? Color{80, 60, 120, 220} : Color{40, 35, 55, 180};
        Color border = selected ? Color{180, 140, 220, 255} : Color{70, 65, 85, 200};

        platform.drawRect({btnX, btnY, btnW, 44.0f}, bg, border, 2.0f);

        if (font) {
            Color tc = selected ? Color{255, 220, 150, 255} : Color{160, 160, 180, 255};
            platform.drawText(*font, labels[i],
                              {btnX + 44.0f, btnY + 10.0f}, 20, tc);
        }

        if (selected && font) {
            float bob = std::sin(m_elapsed * 4.0f) * 4.0f;
            platform.drawText(*font, ">",
                              {btnX - 24.0f + bob, btnY + 10.0f}, 20,
                              Color{255, 220, 150, 255});
        }
    }
}

void GameOverScene::selectItem(int index) {
    m_selectedItem = ((index % GO_COUNT) + GO_COUNT) % GO_COUNT;
}

void GameOverScene::confirmSelection() {
    switch (m_selectedItem) {
        case GO_RETRY: {
            // Pop GameOver + HUD + GameScene, push fresh GameScene
            m_game.scenes().pop();   // GameOverScene
            m_game.scenes().pop();   // HUDScene
            auto scene = std::make_unique<GameScene>(m_game);
            scene->setGameMode(m_mode);
            m_game.scenes().replace(std::move(scene));
            break;
        }
        case GO_MENU:
            m_game.scenes().pop();   // GameOverScene
            m_game.scenes().pop();   // HUDScene
            m_game.scenes().replace(std::make_unique<MenuScene>(m_game));
            break;
    }
}
