#include "scenes/MenuScene.hpp"
#include "scenes/LevelSelectScene.hpp"
#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/InputManager.hpp"
#include "core/ResourceManager.hpp"
#include "audio/AudioManager.hpp"
#include "utils/Logger.hpp"

#include <cmath>

MenuScene::MenuScene(Game& game) : m_game(game) {}

void MenuScene::onEnter() {
    m_selectedItem = MENU_SOLO;
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
        float baseY = 280.0f;
        float itemH = 48.0f;
        float btnW = 300.0f;
        float btnX = (screenW - btnW) * 0.5f;

        for (int i = 0; i < MENU_COUNT; ++i) {
            float btnY = baseY + static_cast<float>(i) * itemH;
            Rect btnRect = {btnX, btnY, btnW, 40.0f};
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
    platform.clear(Color{40, 44, 52, 255}); // Dark pixel-art background

    float screenW = static_cast<float>(Config::WINDOW_WIDTH);

    auto font = ResourceManager::instance().getFont("main");

    // Animated title
    if (font) {
        float titleX = screenW * 0.5f - 180.0f;
        float titleY = 80.0f + m_titleBob;
        platform.drawText(*font, Config::GAME_TITLE,
                          {titleX, titleY}, 48,
                          Color{78, 205, 196, 255}); // Teal

        // Subtitle
        platform.drawText(*font, "Pixel Adventure Platformer",
                          {titleX + 10.0f, titleY + 60.0f}, 16,
                          Color{255, 255, 255, 180});
    }

    // Menu items
    const char* labels[MENU_COUNT] = {
        "Solo",
        "2 Player Alternating",
        "2 Player Co-op",
        "4 Player Co-op",
        "4 Player VS",
        "Quit"
    };
    float baseY = 280.0f;
    float itemH = 48.0f;
    float btnW = 300.0f;
    float btnX = (screenW - btnW) * 0.5f;

    for (int i = 0; i < MENU_COUNT; ++i) {
        float btnY = baseY + static_cast<float>(i) * itemH;
        bool selected = (i == m_selectedItem);

        Color bgColor = selected ? Color{78, 205, 196, 255} : Color{60, 64, 72, 200};
        Color borderColor = selected ? Color{255, 255, 255, 255} : Color{100, 104, 112, 200};

        platform.drawRect({btnX, btnY, btnW, 40.0f}, bgColor, borderColor, 2.0f);

        if (font) {
            Color textColor = selected ? Color{40, 44, 52, 255} : Color{200, 200, 210, 255};
            platform.drawText(*font, labels[i],
                              {btnX + 20.0f, btnY + 10.0f}, 16,
                              textColor);
        }

        if (selected && font) {
            float coinBob = std::sin(m_elapsed * 4.0f) * 4.0f;
            platform.drawText(*font, ">",
                              {btnX - 28.0f + coinBob, btnY + 10.0f}, 18,
                              Color{255, 107, 107, 255});
        }
    }

    // Controls hint
    if (font) {
        platform.drawText(*font, "P1: Arrows+Z/X | P2: WASD+J/K | P3: IJKL+N/M | P4: Numpad",
                          {screenW * 0.5f - 340.0f, 590.0f}, 11,
                          Color{255, 255, 255, 100});
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

    auto createScene = [this](GameMode mode) {
        m_game.scenes().replace(std::make_unique<LevelSelectScene>(m_game, mode));
    };

    switch (m_selectedItem) {
        case MENU_SOLO:     createScene(GameMode::Solo);   break;
        case MENU_2P_ALT:   createScene(GameMode::Alt2P);  break;
        case MENU_2P_COOP:  createScene(GameMode::Coop2P); break;
        case MENU_4P_COOP:  createScene(GameMode::Coop4P); break;
        case MENU_4P_VS:    createScene(GameMode::VS4P);   break;
        case MENU_QUIT:     m_game.platform().close();     break;
    }
}
