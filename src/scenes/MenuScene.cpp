#include "scenes/MenuScene.hpp"
#include "scenes/GameScene.hpp"
#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/InputManager.hpp"
#include "core/ResourceManager.hpp"
#include "audio/AudioManager.hpp"
#include "utils/Logger.hpp"

#include <cmath>

MenuScene::MenuScene(Game& game) : m_game(game) {}

void MenuScene::onEnter() {
    m_selectedItem = MENU_1P;
    m_elapsed = 0.0f;
    m_confirmed = false;
    AudioManager::instance().playMusic("menu_theme", true);
    LOG_INFO("MenuScene entered");
}

void MenuScene::onExit() {}

void MenuScene::handleInput(const InputManager& input) {
    if (input.isJustPressed(Action::Jump) || input.isJustPressed(Action::Confirm)) {
        confirmSelection();
        return;
    }

    if (m_game.platform().isKeyJustPressed(KeyCode::Up)) {
        selectItem(m_selectedItem - 1);
    }
    if (m_game.platform().isKeyJustPressed(KeyCode::Down)) {
        selectItem(m_selectedItem + 1);
    }

    // Mouse/touch click on buttons
    if (input.isPointerDown()) {
        Vec2f pos = input.getPointerPosition();
        float screenW = static_cast<float>(Config::WINDOW_WIDTH);
        float baseY = 340.0f;
        float itemH = 52.0f;
        float btnW = 280.0f;
        float btnX = (screenW - btnW) * 0.5f;

        for (int i = 0; i < MENU_COUNT; ++i) {
            float btnY = baseY + static_cast<float>(i) * itemH;
            Rect btnRect = {btnX, btnY, btnW, 44.0f};
            if (btnRect.contains(pos)) {
                m_selectedItem = i;
                confirmSelection();
                return;
            }
        }
    }
}

void MenuScene::update(float dt) {
    m_elapsed += dt;
    m_titleBob = std::sin(m_elapsed * 2.0f) * 8.0f;
}

void MenuScene::render(IPlatform& platform) {
    platform.clear(Color{92, 148, 252, 255}); // Mario sky blue

    float screenW = static_cast<float>(Config::WINDOW_WIDTH);

    auto font = ResourceManager::instance().getFont("main");

    // Animated title
    if (font) {
        float titleX = screenW * 0.5f - 200.0f;
        float titleY = 120.0f + m_titleBob;
        platform.drawText(*font, Config::GAME_TITLE,
                          {titleX, titleY}, 48,
                          Color{228, 0, 8, 255}); // Mario red

        // Subtitle
        platform.drawText(*font, "Classic Platformer",
                          {titleX + 40.0f, titleY + 60.0f}, 18,
                          Color{255, 255, 255, 200});
    }

    // Menu items
    const char* labels[MENU_COUNT] = {
        "1 Player",
        "2 Players Alternating",
        "2 Players Co-op",
        "Quit"
    };
    float baseY = 340.0f;
    float itemH = 52.0f;
    float btnW = 280.0f;
    float btnX = (screenW - btnW) * 0.5f;

    for (int i = 0; i < MENU_COUNT; ++i) {
        float btnY = baseY + static_cast<float>(i) * itemH;
        bool selected = (i == m_selectedItem);

        // Button background
        Color bgColor = selected ? Color{200, 76, 12, 255} : Color{139, 69, 19, 200};
        Color borderColor = selected ? Color{255, 200, 50, 255} : Color{100, 60, 30, 200};

        platform.drawRect({btnX, btnY, btnW, 44.0f}, bgColor, borderColor, 2.0f);

        // Button label
        if (font) {
            Color textColor = selected ? Color{255, 255, 255, 255} : Color{200, 190, 170, 255};
            platform.drawText(*font, labels[i],
                              {btnX + 20.0f, btnY + 12.0f}, 18,
                              textColor);
        }

        // Selection coin indicator
        if (selected && font) {
            float coinBob = std::sin(m_elapsed * 4.0f) * 4.0f;
            platform.drawText(*font, ">",
                              {btnX - 28.0f + coinBob, btnY + 12.0f}, 18,
                              Color{255, 220, 50, 255});
        }
    }

    // Controls hint
    if (font) {
        platform.drawText(*font, "P1: Arrows + Z/X  |  P2: WASD + J/K",
                          {screenW * 0.5f - 200.0f, 570.0f}, 14,
                          Color{255, 255, 255, 120});
        platform.drawText(*font, "v" + Config::GAME_VERSION,
                          {screenW - 80.0f, 690.0f}, 12,
                          Color{255, 255, 255, 80});
    }
}

void MenuScene::selectItem(int index) {
    int newItem = ((index % MENU_COUNT) + MENU_COUNT) % MENU_COUNT;
    if (newItem != m_selectedItem) {
        m_selectedItem = newItem;
        AudioManager::instance().playSound("menu_select");
    }
}

void MenuScene::confirmSelection() {
    if (m_confirmed) return;
    m_confirmed = true;
    AudioManager::instance().playSound("menu_confirm");

    switch (m_selectedItem) {
        case MENU_1P: {
            auto scene = std::make_unique<GameScene>(m_game);
            scene->setPlayerMode(1, false);
            m_game.scenes().replace(std::move(scene));
            break;
        }
        case MENU_2P_ALT: {
            auto scene = std::make_unique<GameScene>(m_game);
            scene->setPlayerMode(2, false);
            m_game.scenes().replace(std::move(scene));
            break;
        }
        case MENU_2P_COOP: {
            auto scene = std::make_unique<GameScene>(m_game);
            scene->setPlayerMode(2, true);
            m_game.scenes().replace(std::move(scene));
            break;
        }
        case MENU_QUIT:
            m_game.platform().close();
            break;
    }
}
