#include "scenes/LevelCompleteScene.hpp"
#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/InputManager.hpp"
#include "core/ResourceManager.hpp"
#include "audio/AudioManager.hpp"

#include <cmath>
#include <string>

LevelCompleteScene::LevelCompleteScene(Game& game, GameStatePtr state,
                                       std::function<void()> onContinue)
    : m_game(game)
    , m_state(std::move(state))
    , m_onContinue(std::move(onContinue))
{}

void LevelCompleteScene::onEnter() {
    m_elapsed = 0.0f;
    m_continued = false;
    AudioManager::instance().playSound("level_complete");
}

void LevelCompleteScene::onExit() {}

void LevelCompleteScene::handleInput(const InputManager& input) {
    if (m_elapsed < 1.0f || m_continued) return;

    if (input.isJustPressed(Action::Confirm) || input.isJustPressed(Action::Jump)) {
        m_continued = true;
        m_game.scenes().pop(); // pop this overlay
        if (m_onContinue) m_onContinue();
    }
}

void LevelCompleteScene::update(float dt) {
    m_elapsed += dt;

    if (!m_continued && m_elapsed >= AUTO_ADVANCE_DELAY) {
        m_continued = true;
        m_game.scenes().pop();
        if (m_onContinue) m_onContinue();
    }
}

void LevelCompleteScene::render(IPlatform& platform) {
    float screenW = static_cast<float>(Config::WINDOW_WIDTH);
    float screenH = static_cast<float>(Config::WINDOW_HEIGHT);

    // Dim overlay
    platform.drawRect({0, 0, screenW, screenH}, Color{0, 0, 0, 180});

    auto font = ResourceManager::instance().getFont("main");
    if (!font) return;
    FontHandle f = *font;

    // Title with bounce
    float titleY = 180.0f + std::sin(m_elapsed * 2.0f) * 6.0f;
    platform.drawText(f, "LEVEL COMPLETE!",
                      {screenW * 0.5f - 160.0f, titleY}, 36,
                      Color{100, 255, 150, 255});

    if (!m_state) return;

    // World display
    platform.drawText(f, m_state->worldDisplay,
                      {screenW * 0.5f - 20.0f, titleY + 50.0f}, 22,
                      Color{255, 220, 50, 255});

    // Score summary
    float sumY = 320.0f;
    int numActive = m_state->numActivePlayers();
    int totalScore = 0;

    for (int i = 0; i < numActive; ++i) {
        const auto& ps = m_state->players[static_cast<size_t>(i)];
        totalScore += ps.score;
    }

    platform.drawText(f, "SCORE",
                      {screenW * 0.5f - 40.0f, sumY}, 16,
                      Color{255, 255, 255, 200});
    platform.drawText(f, std::to_string(totalScore),
                      {screenW * 0.5f - 50.0f, sumY + 26.0f}, 32,
                      Color{255, 220, 50, 255});

    // Time bonus
    int timeBonus = static_cast<int>(m_state->levelTimer) * 50;
    platform.drawText(f, "TIME BONUS  +" + std::to_string(timeBonus),
                      {screenW * 0.5f - 100.0f, sumY + 76.0f}, 14,
                      Color{180, 255, 180, 220});

    // Continue hint
    if (m_elapsed >= 1.0f) {
        float alpha = static_cast<float>(std::fmod(m_elapsed, 1.5) > 0.75 ? 180 : 255);
        platform.drawText(f, "Press ENTER to continue",
                          {screenW * 0.5f - 120.0f, 500.0f}, 12,
                          Color{255, 255, 255, static_cast<uint8_t>(alpha)});
    }
}
