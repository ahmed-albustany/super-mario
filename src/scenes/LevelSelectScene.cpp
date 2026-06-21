#include "scenes/LevelSelectScene.hpp"
#include "scenes/GameScene.hpp"
#include "scenes/MenuScene.hpp"
#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/ResourceManager.hpp"
#include "audio/AudioManager.hpp"
#include "utils/Logger.hpp"

#include <cmath>

LevelSelectScene::LevelSelectScene(Game& game, GameMode mode)
    : m_game(game), m_mode(mode) {}

void LevelSelectScene::onEnter() {
    m_selectedLevel = 0;
    m_elapsed = 0.0f;
    m_confirmed = false;
    LOG_INFO("LevelSelectScene entered");
}

void LevelSelectScene::onExit() {}

void LevelSelectScene::handleInput(const InputManager& input) {
    if (m_confirmed) return;

    if (input.isJustPressed(Action::Jump) || input.isJustPressed(Action::Confirm)) {
        confirmSelection();
        return;
    }

    // Back to menu
    if (input.isJustPressed(Action::Pause)) {
        m_game.scenes().replace(std::make_unique<MenuScene>(m_game));
        return;
    }

    // Grid navigation: 5 columns x 2 rows
    if (m_game.platform().isKeyJustPressed(KeyCode::Right)) {
        selectLevel(m_selectedLevel + 1);
    }
    if (m_game.platform().isKeyJustPressed(KeyCode::Left)) {
        selectLevel(m_selectedLevel - 1);
    }
    if (m_game.platform().isKeyJustPressed(KeyCode::Down)) {
        selectLevel(m_selectedLevel + 5);
    }
    if (m_game.platform().isKeyJustPressed(KeyCode::Up)) {
        selectLevel(m_selectedLevel - 5);
    }

    // Mouse/touch click
    if (input.isPointerDown()) {
        Vec2f pos = input.getPointerPosition();
        float screenW = static_cast<float>(Config::WINDOW_WIDTH);
        float gridW = 5 * 120.0f;
        float startX = (screenW - gridW) * 0.5f;
        float startY = 240.0f;

        for (int i = 0; i < GameState::TOTAL_LEVELS; ++i) {
            int col = i % 5;
            int row = i / 5;
            float bx = startX + static_cast<float>(col) * 120.0f;
            float by = startY + static_cast<float>(row) * 120.0f;
            Rect btnRect = {bx, by, 100.0f, 100.0f};
            if (btnRect.contains(pos)) {
                m_selectedLevel = i;
                confirmSelection();
                return;
            }
        }
    }
}

void LevelSelectScene::update(float dt) {
    m_elapsed += dt;
}

void LevelSelectScene::render(IPlatform& platform) {
    platform.clear(Color{40, 44, 52, 255});

    float screenW = static_cast<float>(Config::WINDOW_WIDTH);
    auto font = ResourceManager::instance().getFont("main");

    // Title
    if (font) {
        platform.drawText(*font, "SELECT LEVEL",
                          {screenW * 0.5f - 120.0f, 60.0f}, 36,
                          Color{78, 205, 196, 255});

        // Mode subtitle
        const char* modeNames[] = {"Solo", "2P Alternating", "2P Co-op", "4P Co-op", "4P VS"};
        int modeIdx = static_cast<int>(m_mode);
        if (modeIdx >= 0 && modeIdx < 5) {
            platform.drawText(*font, modeNames[modeIdx],
                              {screenW * 0.5f - 60.0f, 120.0f}, 16,
                              Color{255, 255, 255, 150});
        }
    }

    // Level grid: 5 columns x 2 rows
    float gridW = 5 * 120.0f;
    float startX = (screenW - gridW) * 0.5f;
    float startY = 240.0f;

    for (int i = 0; i < GameState::TOTAL_LEVELS; ++i) {
        int col = i % 5;
        int row = i / 5;
        float bx = startX + static_cast<float>(col) * 120.0f;
        float by = startY + static_cast<float>(row) * 120.0f;
        bool selected = (i == m_selectedLevel);

        // Background
        Color bgColor = selected ? Color{78, 205, 196, 255} : Color{60, 64, 72, 220};
        Color borderColor = selected ? Color{255, 255, 255, 255} : Color{100, 104, 112, 200};
        float borderW = selected ? 3.0f : 1.0f;

        platform.drawRect({bx, by, 100.0f, 100.0f}, bgColor, borderColor, borderW);

        if (font) {
            // World name (e.g., "1-1")
            Color textColor = selected ? Color{40, 44, 52, 255} : Color{200, 200, 210, 255};
            const auto& worldName = GameState::WORLD_NAMES[static_cast<size_t>(i)];
            platform.drawText(*font, worldName,
                              {bx + 30.0f, by + 25.0f}, 28, textColor);

            // Level number
            std::string levelNum = "Level " + std::to_string(i + 1);
            Color subColor = selected ? Color{40, 44, 52, 180} : Color{150, 150, 160, 180};
            platform.drawText(*font, levelNum,
                              {bx + 18.0f, by + 70.0f}, 12, subColor);
        }

        // Selection indicator
        if (selected && font) {
            float bob = std::sin(m_elapsed * 4.0f) * 4.0f;
            platform.drawText(*font, ">",
                              {bx - 18.0f + bob, by + 30.0f}, 24,
                              Color{255, 107, 107, 255});
        }
    }

    // Controls hint
    if (font) {
        platform.drawText(*font, "Arrow Keys: Select  |  Z/Enter: Start  |  ESC: Back",
                          {screenW * 0.5f - 240.0f, 550.0f}, 14,
                          Color{255, 255, 255, 100});
    }
}

void LevelSelectScene::selectLevel(int index) {
    if (index < 0 || index >= GameState::TOTAL_LEVELS) return;
    if (index != m_selectedLevel) {
        m_selectedLevel = index;
        AudioManager::instance().playSound("menu_select");
    }
}

void LevelSelectScene::confirmSelection() {
    if (m_confirmed) return;
    m_confirmed = true;
    AudioManager::instance().playSound("menu_confirm");

    auto scene = std::make_unique<GameScene>(m_game);
    scene->setGameMode(m_mode);
    scene->setStartLevel(m_selectedLevel);
    m_game.scenes().replace(std::move(scene));
}
