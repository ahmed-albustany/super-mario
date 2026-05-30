#include "scenes/PauseScene.hpp"
#include "scenes/MenuScene.hpp"
#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/InputManager.hpp"
#include "core/ResourceManager.hpp"
#include "audio/AudioManager.hpp"

#include <cmath>

PauseScene::PauseScene(Game& game) : m_game(game) {}

void PauseScene::onEnter() {
    m_selectedItem = PAUSE_RESUME;
    m_elapsed = 0.0f;
    AudioManager::instance().playSound("pause");
}

void PauseScene::onExit() {}

void PauseScene::handleInput(const InputManager& input) {
    // Unpause with Pause key or Back
    if (input.isJustPressed(Action::Pause) || input.isJustPressed(Action::Back)) {
        m_game.scenes().pop();
        return;
    }

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
        float baseY = 340.0f;
        float itemH = 60.0f;
        float btnW = 200.0f;
        float btnX = (screenW - btnW) * 0.5f;

        for (int i = 0; i < PAUSE_COUNT; ++i) {
            float btnY = baseY + static_cast<float>(i) * itemH;
            if (Rect{btnX, btnY, btnW, 44.0f}.contains(pos)) {
                m_selectedItem = i;
                confirmSelection();
                return;
            }
        }
    }
}

void PauseScene::update(float dt) {
    m_elapsed += dt;
}

void PauseScene::render(IPlatform& platform) {
    float screenW = static_cast<float>(Config::WINDOW_WIDTH);
    float screenH = static_cast<float>(Config::WINDOW_HEIGHT);

    // Dim overlay
    platform.drawRect({0, 0, screenW, screenH}, Color{0, 0, 0, 150});

    auto font = ResourceManager::instance().getFont("main");

    // Title
    if (font) {
        platform.drawText(*font, "PAUSED",
                          {screenW * 0.5f - 70.0f, 240.0f}, 40,
                          Color{255, 255, 255, 255});
    }

    // Buttons
    const char* labels[PAUSE_COUNT] = {"Resume", "Quit to Menu"};
    float baseY = 340.0f;
    float itemH = 60.0f;
    float btnW = 200.0f;
    float btnX = (screenW - btnW) * 0.5f;

    for (int i = 0; i < PAUSE_COUNT; ++i) {
        float btnY = baseY + static_cast<float>(i) * itemH;
        bool selected = (i == m_selectedItem);

        Color bg     = selected ? Color{80, 60, 120, 220} : Color{40, 35, 55, 180};
        Color border = selected ? Color{180, 140, 220, 255} : Color{70, 65, 85, 200};

        platform.drawRect({btnX, btnY, btnW, 44.0f}, bg, border, 2.0f);

        if (font) {
            Color tc = selected ? Color{255, 220, 150, 255} : Color{160, 160, 180, 255};
            platform.drawText(*font, labels[i],
                              {btnX + 40.0f, btnY + 10.0f}, 20, tc);
        }

        if (selected && font) {
            float bob = std::sin(m_elapsed * 4.0f) * 4.0f;
            platform.drawText(*font, ">",
                              {btnX - 24.0f + bob, btnY + 10.0f}, 20,
                              Color{255, 220, 150, 255});
        }
    }
}

void PauseScene::selectItem(int index) {
    m_selectedItem = ((index % PAUSE_COUNT) + PAUSE_COUNT) % PAUSE_COUNT;
}

void PauseScene::confirmSelection() {
    switch (m_selectedItem) {
        case PAUSE_RESUME:
            m_game.scenes().pop();
            break;
        case PAUSE_QUIT:
            // Pop pause + HUD + game scene, push menu
            m_game.scenes().pop();  // pause
            m_game.scenes().pop();  // HUD
            m_game.scenes().replace(std::make_unique<MenuScene>(m_game));
            break;
    }
}
