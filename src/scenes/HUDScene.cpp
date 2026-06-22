#include "scenes/HUDScene.hpp"
#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/ResourceManager.hpp"
#include "core/Events.hpp"

#include <string>
#include <cmath>

HUDScene::HUDScene(Game& /*game*/, GameStatePtr state)
    : m_state(std::move(state))
{}

void HUDScene::onEnter() {}

void HUDScene::onExit() {}

void HUDScene::handleInput(const InputManager& /*input*/) {}

void HUDScene::update(float /*dt*/) {}

void HUDScene::render(IPlatform& platform) {
    if (!m_state) return;

    auto font = ResourceManager::instance().getFont("main");
    if (!font) return;

    FontHandle f = *font;
    float pad = 16.0f;
    float screenW = static_cast<float>(Config::WINDOW_WIDTH);
    float screenH = static_cast<float>(Config::WINDOW_HEIGHT);
    const auto& gs = *m_state;

    int numActive = gs.numActivePlayers();
    // Character labels
    static const char* CHAR_NAMES[] = {"MASK DUDE", "NINJA FROG", "PINK MAN", "VIRTUAL GUY"};
    // Player colors for HUD elements
    static const Color PLAYER_COLORS[] = {
        {78, 205, 196, 255},    // P1: Teal
        {255, 107, 107, 255},   // P2: Red
        {255, 182, 193, 255},   // P3: Pink
        {130, 130, 255, 255}    // P4: Blue
    };

    if (numActive == 1 || gs.mode == GameMode::Alt2P) {
        // ---- Single player / Alternating HUD ----
        int activeIdx = gs.currentPlayer;
        const auto& ps = gs.players[static_cast<size_t>(activeIdx)];

        // Player name (top-left)
        Color labelColor = PLAYER_COLORS[static_cast<size_t>(activeIdx)];
        platform.drawText(f, CHAR_NAMES[static_cast<size_t>(activeIdx)],
                          {pad, pad}, 14, labelColor);

        // Score (top-left, under name)
        std::string scoreStr = std::to_string(ps.score);
        while (scoreStr.size() < 6) scoreStr = "0" + scoreStr;
        platform.drawText(f, scoreStr, {pad, pad + 18.0f}, 22, {255, 255, 255, 255});

        // Fruits collected (top-center-left)
        std::string fruitStr = "FRUITS x" + std::to_string(ps.fruitsCollected);
        platform.drawText(f, fruitStr, {280.0f, pad + 18.0f}, 18, {255, 220, 50, 255});

        // WORLD (top-center)
        float worldX = screenW * 0.5f - 40.0f;
        platform.drawText(f, "WORLD", {worldX, pad}, 14, {255, 255, 255, 255});
        platform.drawText(f, gs.worldDisplay, {worldX + 8.0f, pad + 18.0f}, 22, {255, 255, 255, 255});

        // TIME (top-right, inset enough so label + 3-digit number stay visible)
        float timeX = screenW - 200.0f;
        int seconds = static_cast<int>(std::ceil(gs.levelTimer));
        std::string timeStr = std::to_string(seconds);
        Color timerColor = (gs.levelTimer < 100.0f) ? Color{255, 80, 80, 255} : Color{255, 255, 255, 255};
        platform.drawText(f, "TIME", {timeX, pad}, 14, {255, 255, 255, 255});
        platform.drawText(f, timeStr, {timeX + 10.0f, pad + 18.0f}, 22, timerColor);

        // Lives (bottom-left) with character icon placeholder
        Color livesColor = PLAYER_COLORS[static_cast<size_t>(activeIdx)];
        platform.drawRect({pad, screenH - 40.0f, 20.0f, 20.0f}, livesColor, livesColor, 0.0f);
        std::string livesStr = "x" + std::to_string(ps.lives);
        platform.drawText(f, livesStr, {pad + 26.0f, screenH - 38.0f}, 18, {255, 255, 255, 230});

        // In alternating mode, show the other player's status dimmed on the right
        if (gs.mode == GameMode::Alt2P) {
            int otherIdx = 1 - activeIdx;
            const auto& otherPs = gs.players[static_cast<size_t>(otherIdx)];
            Color dimColor = {160, 160, 160, 140};
            float p2X = screenW - 200.0f;
            platform.drawText(f, CHAR_NAMES[static_cast<size_t>(otherIdx)],
                              {p2X, pad}, 12, dimColor);
            std::string p2Score = std::to_string(otherPs.score);
            while (p2Score.size() < 6) p2Score = "0" + p2Score;
            platform.drawText(f, p2Score, {p2X, pad + 16.0f}, 16, dimColor);
        }
    } else {
        // ---- Multi-player simultaneous HUD (2P co-op / 4P co-op / 4P VS) ----

        // TIME (top-center)
        float worldX = screenW * 0.5f - 40.0f;
        int seconds = static_cast<int>(std::ceil(gs.levelTimer));
        std::string timeStr = std::to_string(seconds);
        Color timerColor = (gs.levelTimer < 100.0f) ? Color{255, 80, 80, 255} : Color{255, 255, 255, 255};
        platform.drawText(f, "TIME", {worldX, pad}, 14, {255, 255, 255, 255});
        platform.drawText(f, timeStr, {worldX + 10.0f, pad + 18.0f}, 22, timerColor);

        // WORLD (just below time, centered)
        platform.drawText(f, gs.worldDisplay, {worldX + 8.0f, pad + 44.0f}, 14, {255, 255, 255, 180});

        // VS label
        if (gs.isVSMode()) {
            platform.drawText(f, "VS MODE", {screenW * 0.5f - 40.0f, pad + 62.0f}, 12, {255, 107, 107, 200});
        }

        // Player panels — spread across the bottom of the screen
        float panelW = screenW / static_cast<float>(numActive);
        float panelY = screenH - 54.0f;

        for (int i = 0; i < numActive; ++i) {
            const auto& ps = gs.players[static_cast<size_t>(i)];
            float px = static_cast<float>(i) * panelW + 8.0f;
            Color pColor = PLAYER_COLORS[static_cast<size_t>(i)];

            // Dim dead players
            if (!ps.isAlive || ps.lives <= 0) {
                pColor.a = 100;
            }

            // Player name
            std::string pLabel = "P" + std::to_string(i + 1);
            platform.drawText(f, pLabel, {px, panelY}, 12, pColor);

            // Score
            std::string pScore = std::to_string(ps.score);
            platform.drawText(f, pScore, {px, panelY + 14.0f}, 14, pColor);

            // Lives
            std::string pLives = "x" + std::to_string(ps.lives);
            platform.drawText(f, pLives, {px + 80.0f, panelY + 14.0f}, 14, {255, 255, 255, 200});

            // Fruits
            std::string pFruits = std::to_string(ps.fruitsCollected);
            platform.drawText(f, pFruits, {px + 110.0f, panelY + 14.0f}, 14, {255, 220, 50, 200});
        }

        // Also show each player's score at the top in their corner
        if (numActive >= 2) {
            // P1 top-left
            const auto& p1 = gs.players[0];
            platform.drawText(f, "P1", {pad, pad}, 12, PLAYER_COLORS[0]);
            std::string s1 = std::to_string(p1.score);
            while (s1.size() < 6) s1 = "0" + s1;
            platform.drawText(f, s1, {pad, pad + 14.0f}, 16, PLAYER_COLORS[0]);

            // P2 top-right
            const auto& p2 = gs.players[1];
            float p2X = screenW - 120.0f;
            platform.drawText(f, "P2", {p2X, pad}, 12, PLAYER_COLORS[1]);
            std::string s2 = std::to_string(p2.score);
            while (s2.size() < 6) s2 = "0" + s2;
            platform.drawText(f, s2, {p2X, pad + 14.0f}, 16, PLAYER_COLORS[1]);
        }

        if (numActive >= 4) {
            // P3 below P1
            const auto& p3 = gs.players[2];
            platform.drawText(f, "P3", {pad, pad + 36.0f}, 12, PLAYER_COLORS[2]);
            std::string s3 = std::to_string(p3.score);
            while (s3.size() < 6) s3 = "0" + s3;
            platform.drawText(f, s3, {pad, pad + 50.0f}, 16, PLAYER_COLORS[2]);

            // P4 below P2
            const auto& p4 = gs.players[3];
            float p4X = screenW - 120.0f;
            platform.drawText(f, "P4", {p4X, pad + 36.0f}, 12, PLAYER_COLORS[3]);
            std::string s4 = std::to_string(p4.score);
            while (s4.size() < 6) s4 = "0" + s4;
            platform.drawText(f, s4, {p4X, pad + 50.0f}, 16, PLAYER_COLORS[3]);
        }
    }
}
