#include "scenes/HUDScene.hpp"
#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/ResourceManager.hpp"
#include "core/Events.hpp"

#include <string>
#include <cmath>

HUDScene::HUDScene(Game& game, const int& score, const int& lives,
                     const int& coins, const int& gems, const float& timer)
    : m_game(game)
    , m_score(score)
    , m_lives(lives)
    , m_coins(coins)
    , m_gems(gems)
    , m_timer(timer)
{}

void HUDScene::onEnter() {
    // Subscribe to power-up events to show the timer bar
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

void HUDScene::handleInput(const InputManager& /*input*/) {
    // HUD doesn't consume input — falls through to GameScene
}

void HUDScene::update(float dt) {
    // Tick power-up display timer
    if (m_powerUpActive) {
        m_powerUpTimer -= dt;
        if (m_powerUpTimer <= 0.0f) {
            m_powerUpTimer  = 0.0f;
            m_powerUpActive = false;
        }
    }
}

void HUDScene::render(IPlatform& platform) {
    auto font = ResourceManager::instance().getFont("main");
    if (!font) return;

    FontHandle f = *font;
    float pad = 16.0f;

    // ---- Score (top-left) ----
    platform.drawText(f, "SCORE", {pad, pad}, 14, Color{180, 180, 200, 255});
    platform.drawText(f, std::to_string(m_score), {pad, pad + 18.0f}, 24,
                      Color::White());

    // ---- Lives (below score) ----
    platform.drawText(f, "LIVES", {pad, pad + 56.0f}, 14, Color{180, 180, 200, 255});
    // Draw lives as small squares
    for (int i = 0; i < m_lives; ++i) {
        float x = pad + static_cast<float>(i) * 22.0f;
        platform.drawRect({x, pad + 74.0f, 16.0f, 16.0f},
                          Color{255, 80, 80, 255},
                          Color{255, 200, 200, 255}, 1.0f);
    }

    // ---- Coins (top-center-left) ----
    float coinX = 220.0f;
    platform.drawText(f, "COINS", {coinX, pad}, 14, Color{180, 180, 200, 255});
    platform.drawText(f, std::to_string(m_coins), {coinX, pad + 18.0f}, 22,
                      Color{255, 220, 50, 255});

    // ---- Gems (next to coins) ----
    float gemX = 360.0f;
    platform.drawText(f, "GEMS", {gemX, pad}, 14, Color{180, 180, 200, 255});
    platform.drawText(f, std::to_string(m_gems), {gemX, pad + 18.0f}, 22,
                      Color{100, 220, 255, 255});

    // ---- Timer (top-right) ----
    float screenW = static_cast<float>(Config::WINDOW_WIDTH);
    int seconds = static_cast<int>(std::ceil(m_timer));
    int mins = seconds / 60;
    int secs = seconds % 60;
    std::string timeStr = std::to_string(mins) + ":"
        + (secs < 10 ? "0" : "") + std::to_string(secs);

    Color timerColor = (m_timer < 30.0f) ? Color{255, 80, 80, 255} : Color::White();
    platform.drawText(f, "TIME", {screenW - 120.0f, pad}, 14,
                      Color{180, 180, 200, 255});
    platform.drawText(f, timeStr, {screenW - 120.0f, pad + 18.0f}, 24, timerColor);

    // ---- Power-up timer bar (bottom-center, only when active) ----
    if (m_powerUpActive && m_powerUpDuration > 0.0f) {
        float barW = 300.0f;
        float barH = 12.0f;
        float barX = (screenW - barW) * 0.5f;
        float barY = static_cast<float>(Config::WINDOW_HEIGHT) - 50.0f;
        float fill = m_powerUpTimer / m_powerUpDuration;

        // Background
        platform.drawRect({barX, barY, barW, barH},
                          Color{30, 30, 50, 200}, Color{100, 100, 140, 255}, 1.0f);

        // Fill (drains left to right)
        float fillW = (barW - 2.0f) * fill;
        Color barColor = (fill > 0.3f) ? Color{100, 220, 255, 255} : Color{255, 100, 80, 255};
        platform.drawRect({barX + 1.0f, barY + 1.0f, fillW, barH - 2.0f}, barColor);

        // Label
        platform.drawText(f, "POWER", {barX, barY - 18.0f}, 12,
                          Color{100, 220, 255, 200});
    }
}
