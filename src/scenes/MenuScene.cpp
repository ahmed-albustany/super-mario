#include "scenes/MenuScene.hpp"
#include "scenes/GameScene.hpp"
#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/InputManager.hpp"
#include "core/ResourceManager.hpp"
#include "utils/Logger.hpp"

#include <cmath>

MenuScene::MenuScene(Game& game) : m_game(game) {}

void MenuScene::onEnter() {
    m_selectedItem = MENU_PLAY;
    m_elapsed = 0.0f;
    m_confirmed = false;
    LOG_INFO("MenuScene entered");
}

void MenuScene::onExit() {}

void MenuScene::handleInput(const InputManager& input) {
    if (input.isJustPressed(Action::Jump) || input.isJustPressed(Action::Confirm)) {
        confirmSelection();
        return;
    }

    // Navigate menu items
    if (input.isJustPressed(Action::MoveLeft) || input.isJustPressed(Action::Back)) {
        // Quit shortcut
    }

    // Up/Down to move selection (reuse MoveLeft for up since we don't have explicit Up action on InputManager)
    // We'll check platform keys directly — Up and Down map to KeyCode::Up/Down
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
        float baseY = 380.0f;
        float itemH = 60.0f;
        float btnW = 200.0f;
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
    platform.clear(Color{15, 12, 25, 255});

    float screenW = static_cast<float>(Config::WINDOW_WIDTH);

    auto font = ResourceManager::instance().getFont("main");

    // Animated title
    if (font) {
        float titleX = screenW * 0.5f - 200.0f;
        float titleY = 160.0f + m_titleBob;
        platform.drawText(*font, Config::GAME_TITLE,
                          {titleX, titleY}, 48,
                          Color{220, 180, 100, 255});

        // Subtitle
        platform.drawText(*font, "A Platformer Adventure",
                          {titleX + 20.0f, titleY + 60.0f}, 18,
                          Color{140, 140, 160, 255});
    }

    // Menu items
    const char* labels[MENU_COUNT] = {"Play", "Quit"};
    float baseY = 380.0f;
    float itemH = 60.0f;
    float btnW = 200.0f;
    float btnX = (screenW - btnW) * 0.5f;

    for (int i = 0; i < MENU_COUNT; ++i) {
        float btnY = baseY + static_cast<float>(i) * itemH;
        bool selected = (i == m_selectedItem);

        // Button background
        Color bgColor = selected ? Color{80, 60, 120, 255} : Color{40, 35, 55, 200};
        Color borderColor = selected ? Color{180, 140, 220, 255} : Color{70, 65, 85, 200};

        platform.drawRect({btnX, btnY, btnW, 44.0f}, bgColor, borderColor, 2.0f);

        // Button label
        if (font) {
            Color textColor = selected ? Color{255, 220, 150, 255} : Color{160, 160, 180, 255};
            platform.drawText(*font, labels[i],
                              {btnX + btnW * 0.5f - 30.0f, btnY + 10.0f}, 22,
                              textColor);
        }

        // Selection arrow
        if (selected && font) {
            float arrowBob = std::sin(m_elapsed * 4.0f) * 4.0f;
            platform.drawText(*font, ">",
                              {btnX - 28.0f + arrowBob, btnY + 10.0f}, 22,
                              Color{255, 220, 150, 255});
        }
    }

    // Controls hint
    if (font) {
        platform.drawText(*font, "Arrow Keys + Z to select   |   Z = Jump   X = Dash",
                          {screenW * 0.5f - 240.0f, 560.0f}, 14,
                          Color{80, 80, 100, 255});
        platform.drawText(*font, "v" + Config::GAME_VERSION,
                          {screenW - 80.0f, 690.0f}, 12,
                          Color{60, 60, 80, 255});
    }
}

void MenuScene::selectItem(int index) {
    m_selectedItem = ((index % MENU_COUNT) + MENU_COUNT) % MENU_COUNT;
}

void MenuScene::confirmSelection() {
    if (m_confirmed) return; // already transitioning
    m_confirmed = true;

    switch (m_selectedItem) {
        case MENU_PLAY:
            m_game.scenes().replace(std::make_unique<GameScene>(m_game));
            break;
        case MENU_QUIT:
            m_game.platform().close();
            break;
    }
}
