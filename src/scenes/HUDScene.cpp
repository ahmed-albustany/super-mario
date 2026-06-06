#include "scenes/HUDScene.hpp"
#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/ResourceManager.hpp"
#include "core/Events.hpp"

#include <string>
#include <cmath>

HUDScene::HUDScene(Game& game, GameStatePtr state)
    : m_game(game)
    , m_state(std::move(state))
{}

void HUDScene::onEnter() {
    m_subPowerUp = m_game.events().subscribe<PowerUpActivatedEvent>(
        [this](const PowerUpActivatedEvent& e) {
            m_powerUpDuration = e.duration;
            m_powerUpTimer    = e.duration;
            m_powerUpActive   = true;
        });
}

void HUDScene::onExit() {
    m_game.events().unsubscribe<PowerUpActivatedEvent>(m_subPowerUp);
}

void HUDScene::handleInput(const InputManager& /*input*/) {}

void HUDScene::update(float dt) {
    if (m_powerUpActive) {
        m_powerUpTimer -= dt;
        if (m_powerUpTimer <= 0.0f) {
            m_powerUpTimer  = 0.0f;
            m_powerUpActive = false;
        }
    }
}

void HUDScene::render(IPlatform& platform) {
    if (!m_state) return;

    auto font = ResourceManager::instance().getFont("main");
    if (!font) return;

    FontHandle f = *font;
    float pad = 16.0f;
    float screenW = static_cast<float>(Config::WINDOW_WIDTH);
    const auto& gs = *m_state;

    // ---- P1: MARIO / SCORE (top-left) ----
    // Highlight active player in alternating mode
    bool p1Active = (gs.currentPlayer == 0);
    Color p1Label = p1Active ? Color{255, 255, 255, 255} : Color{160, 160, 160, 180};
    platform.drawText(f, "MARIO", {pad, pad}, 14, p1Label);
    std::string scoreStr = std::to_string(gs.p1.score);
    while (scoreStr.size() < 6) scoreStr = "0" + scoreStr;
    platform.drawText(f, scoreStr, {pad, pad + 18.0f}, 22, p1Label);

    // ---- COINS (top-center-left) ----
    float coinX = 280.0f;
    std::string coinStr = "x" + std::to_string(gs.current().coins);
    platform.drawText(f, coinStr, {coinX, pad + 18.0f}, 22, Color{255, 220, 50, 255});

    // ---- WORLD (top-center) ----
    float worldX = screenW * 0.5f - 40.0f;
    platform.drawText(f, "WORLD", {worldX, pad}, 14, Color{255, 255, 255, 255});
    platform.drawText(f, gs.worldDisplay, {worldX + 8.0f, pad + 18.0f}, 22, Color{255, 255, 255, 255});

    // ---- TIME (top-right area) ----
    float timeX = (gs.numPlayers == 2) ? screenW * 0.5f + 60.0f : screenW - 200.0f;
    int seconds = static_cast<int>(std::ceil(gs.levelTimer));
    std::string timeStr = std::to_string(seconds);
    Color timerColor = (gs.levelTimer < 100.0f) ? Color{255, 80, 80, 255} : Color{255, 255, 255, 255};
    platform.drawText(f, "TIME", {timeX, pad}, 14, Color{255, 255, 255, 255});
    platform.drawText(f, timeStr, {timeX + 10.0f, pad + 18.0f}, 22, timerColor);

    // ---- P2: LUIGI / SCORE (top-right, only in 2-player) ----
    if (gs.numPlayers == 2) {
        bool p2Active = (gs.currentPlayer == 1);
        Color p2Label = p2Active ? Color{255, 255, 255, 255} : Color{160, 160, 160, 180};
        float p2X = screenW - 120.0f;
        platform.drawText(f, "LUIGI", {p2X, pad}, 14, p2Label);
        std::string p2Score = std::to_string(gs.p2.score);
        while (p2Score.size() < 6) p2Score = "0" + p2Score;
        platform.drawText(f, p2Score, {p2X, pad + 18.0f}, 22, p2Label);
    }

    // ---- LIVES (bottom-left) ----
    std::string livesLabel = (gs.currentPlayer == 0) ? "MARIOx" : "LUIGIx";
    platform.drawText(f, livesLabel + std::to_string(gs.current().lives),
                      {pad, static_cast<float>(Config::WINDOW_HEIGHT) - 36.0f}, 16,
                      Color{255, 255, 255, 200});

    // In co-op, also show P2 lives on the right
    if (gs.numPlayers == 2 && gs.coopMode) {
        std::string p2Lives = "LUIGIx" + std::to_string(gs.p2.lives);
        platform.drawText(f, p2Lives,
                          {screenW - 140.0f, static_cast<float>(Config::WINDOW_HEIGHT) - 36.0f}, 16,
                          Color{255, 255, 255, 200});
    }

    // ---- Star power bar (bottom-center) ----
    if (m_powerUpActive && m_powerUpDuration > 0.0f) {
        float barW = 300.0f;
        float barH = 12.0f;
        float barX = (screenW - barW) * 0.5f;
        float barY = static_cast<float>(Config::WINDOW_HEIGHT) - 50.0f;
        float fill = m_powerUpTimer / m_powerUpDuration;

        platform.drawRect({barX, barY, barW, barH},
                          Color{30, 30, 50, 200}, Color{255, 220, 50, 255}, 1.0f);

        float fillW = (barW - 2.0f) * fill;
        Color barColor = (fill > 0.3f) ? Color{255, 220, 50, 255} : Color{255, 100, 80, 255};
        platform.drawRect({barX + 1.0f, barY + 1.0f, fillW, barH - 2.0f}, barColor);

        platform.drawText(f, "STAR", {barX, barY - 18.0f}, 12,
                          Color{255, 220, 50, 200});
    }
}
