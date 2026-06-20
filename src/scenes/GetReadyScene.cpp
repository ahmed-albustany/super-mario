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

    // Player name based on character type
    static const char* CHAR_NAMES[] = {"MASK DUDE", "NINJA FROG", "PINK MAN", "VIRTUAL GUY"};
    static const Color CHAR_COLORS[] = {
        {78, 205, 196, 255}, {255, 107, 107, 255},
        {255, 182, 193, 255}, {130, 130, 255, 255}
    };

    int idx = m_state->currentPlayer;
    const char* playerName = CHAR_NAMES[static_cast<size_t>(idx)];
    Color nameColor = CHAR_COLORS[static_cast<size_t>(idx)];

    platform.drawText(f, m_state->worldDisplay,
                      {screenW * 0.5f - 50.0f, screenH * 0.5f - 80.0f}, 20,
                      Color{255, 255, 255, 255});

    platform.drawText(f, playerName,
                      {screenW * 0.5f - 80.0f, screenH * 0.5f - 30.0f}, 32,
                      nameColor);

    // Lives display
    const auto& ps = m_state->current();
    std::string livesStr = "x " + std::to_string(ps.lives);
    platform.drawText(f, livesStr,
                      {screenW * 0.5f + 30.0f, screenH * 0.5f + 20.0f}, 24,
                      Color{255, 255, 255, 255});
}
