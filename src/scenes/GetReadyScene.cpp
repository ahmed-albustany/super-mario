#include "scenes/GetReadyScene.hpp"
#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/ResourceManager.hpp"
#include "utils/Logger.hpp"

#include <string>

GetReadyScene::GetReadyScene(Game& game, GameStatePtr state)
    : m_game(game)
    , m_state(std::move(state))
{}

void GetReadyScene::onEnter() {
    m_timer = 0.0f;
    LOG_INFO("GetReadyScene: Player " << (m_state->currentPlayer + 1) << "'s turn");
}

void GetReadyScene::onExit() {}

void GetReadyScene::handleInput(const InputManager& /*input*/) {
    // Ignore input during the interstitial
}

void GetReadyScene::update(float dt) {
    m_timer += dt;
    if (m_timer >= DISPLAY_DURATION) {
        m_game.scenes().pop();
    }
}

void GetReadyScene::render(IPlatform& platform) {
    if (!m_state) return;

    float screenW = static_cast<float>(Config::WINDOW_WIDTH);
    float screenH = static_cast<float>(Config::WINDOW_HEIGHT);

    // Dim overlay
    platform.drawRect({0, 0, screenW, screenH}, Color{0, 0, 0, 200});

    auto font = ResourceManager::instance().getFont("main");
    if (!font) return;

    FontHandle f = *font;

    // "PLAYER X"
    bool isMario = (m_state->currentPlayer == 0);
    std::string playerName = isMario ? "MARIO" : "LUIGI";
    Color nameColor = isMario ? Color{228, 0, 8, 255} : Color{0, 148, 0, 255};

    platform.drawText(f, "WORLD 1-1",
                      {screenW * 0.5f - 50.0f, screenH * 0.5f - 80.0f}, 20,
                      Color{255, 255, 255, 255});

    platform.drawText(f, playerName,
                      {screenW * 0.5f - 50.0f, screenH * 0.5f - 30.0f}, 32,
                      nameColor);

    // Lives display
    const auto& ps = m_state->current();
    std::string livesStr = "x " + std::to_string(ps.lives);
    platform.drawText(f, livesStr,
                      {screenW * 0.5f + 30.0f, screenH * 0.5f + 20.0f}, 24,
                      Color{255, 255, 255, 255});
}
