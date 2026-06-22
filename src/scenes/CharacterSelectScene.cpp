#include "scenes/CharacterSelectScene.hpp"
#include "scenes/LevelSelectScene.hpp"
#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/InputManager.hpp"
#include "core/ResourceManager.hpp"
#include "audio/AudioManager.hpp"

#include <cmath>

static const char* CHAR_NAMES[] = {"MASK DUDE", "NINJA FROG", "PINK MAN", "VIRTUAL GUY"};
static const Color CHAR_COLORS[] = {
    {78, 205, 196, 255},    // Teal
    {255, 107, 107, 255},   // Red
    {255, 182, 193, 255},   // Pink
    {130, 130, 255, 255}    // Blue
};
static const CharacterType CHAR_TYPES[] = {
    CharacterType::MaskDude,
    CharacterType::NinjaFrog,
    CharacterType::PinkMan,
    CharacterType::VirtualGuy
};

CharacterSelectScene::CharacterSelectScene(Game& game, GameMode mode)
    : m_game(game), m_mode(mode) {}

void CharacterSelectScene::onEnter() {
    m_elapsed = 0.0f;
    m_currentPicker = 0;
    m_cursor = 0;
    m_picks = {-1, -1, -1, -1};

    switch (m_mode) {
        case GameMode::Solo:   m_numPickers = 1; break;
        case GameMode::Alt2P:  m_numPickers = 2; break;
        case GameMode::Coop2P: m_numPickers = 2; break;
        case GameMode::Coop4P: m_numPickers = 4; break;
        case GameMode::VS4P:   m_numPickers = 4; break;
    }
}

void CharacterSelectScene::onExit() {}

void CharacterSelectScene::handleInput(const InputManager& input) {
    if (input.isJustPressed(Action::Back)) {
        m_game.scenes().pop();
        return;
    }

    if (m_game.platform().isKeyJustPressed(KeyCode::Left)) {
        int next = (m_cursor - 1 + 4) % 4;
        if (m_cursor != next) {
            m_cursor = next;
            AudioManager::instance().playSound("menu_select");
        }
    }
    if (m_game.platform().isKeyJustPressed(KeyCode::Right)) {
        int next = (m_cursor + 1) % 4;
        if (m_cursor != next) {
            m_cursor = next;
            AudioManager::instance().playSound("menu_select");
        }
    }

    if (input.isJustPressed(Action::Jump) || input.isJustPressed(Action::Confirm)) {
        confirmPick();
        return;
    }

    // Mouse/touch
    if (input.isPointerDown()) {
        Vec2f pos = input.getPointerPosition();
        float screenW = static_cast<float>(Config::WINDOW_WIDTH);
        float cardW = 200.0f;
        float gap = 40.0f;
        float totalW = 4.0f * cardW + 3.0f * gap;
        float startX = (screenW - totalW) * 0.5f;
        float cardY = 250.0f;
        float cardH = 260.0f;

        for (int i = 0; i < 4; ++i) {
            float cx = startX + static_cast<float>(i) * (cardW + gap);
            if (pos.x >= cx && pos.x <= cx + cardW && pos.y >= cardY && pos.y <= cardY + cardH) {
                m_cursor = i;
                confirmPick();
                return;
            }
        }
    }
}

void CharacterSelectScene::update(float dt) {
    m_elapsed += dt;
}

void CharacterSelectScene::render(IPlatform& platform) {
    platform.clear(Color{40, 44, 52, 255});

    auto font = ResourceManager::instance().getFont("main");
    if (!font) return;
    FontHandle f = *font;

    float screenW = static_cast<float>(Config::WINDOW_WIDTH);

    // Title
    std::string title = (m_numPickers > 1)
        ? "PLAYER " + std::to_string(m_currentPicker + 1) + " — CHOOSE CHARACTER"
        : "CHOOSE YOUR CHARACTER";
    float titleW = static_cast<float>(title.size()) * 14.0f;
    platform.drawText(f, title, {(screenW - titleW) * 0.5f, 60.0f}, 28,
                      Color{255, 255, 255, 255});

    // Subtitle hint
    platform.drawText(f, "Left/Right + Z to confirm | ESC to go back",
                      {screenW * 0.5f - 220.0f, 110.0f}, 13,
                      Color{255, 255, 255, 120});

    // Character cards
    float cardW = 200.0f;
    float gap = 40.0f;
    float totalW = 4.0f * cardW + 3.0f * gap;
    float startX = (screenW - totalW) * 0.5f;
    float cardY = 180.0f;
    float cardH = 280.0f;

    for (int i = 0; i < 4; ++i) {
        float cx = startX + static_cast<float>(i) * (cardW + gap);
        bool selected = (i == m_cursor);
        bool taken = isTaken(i);

        // Card background
        Color bg = taken ? Color{60, 60, 60, 150} : (selected ? Color{60, 80, 100, 220} : Color{50, 54, 62, 200});
        Color border = taken ? Color{80, 80, 80, 180} : (selected ? CHAR_COLORS[static_cast<size_t>(i)] : Color{80, 84, 92, 200});
        float borderW = selected ? 3.0f : 1.5f;
        platform.drawRect({cx, cardY, cardW, cardH}, bg, border, borderW);

        // Character color preview (large colored square as placeholder for sprite)
        Color previewColor = CHAR_COLORS[static_cast<size_t>(i)];
        if (taken) previewColor.a = 80;
        float previewSize = 80.0f;
        float previewX = cx + (cardW - previewSize) * 0.5f;
        float previewY = cardY + 40.0f;
        platform.drawRect({previewX, previewY, previewSize, previewSize}, previewColor, previewColor, 0.0f);

        // Idle animation bob for selected character
        if (selected && !taken) {
            float bob = std::sin(m_elapsed * 3.0f) * 6.0f;
            platform.drawRect({previewX + 10.0f, previewY + 10.0f + bob, 60.0f, 60.0f},
                              Color{255, 255, 255, 60}, Color{255, 255, 255, 60}, 0.0f);
        }

        // Character name
        Color nameColor = taken ? Color{120, 120, 120, 180} : CHAR_COLORS[static_cast<size_t>(i)];
        float nameW = static_cast<float>(std::string(CHAR_NAMES[i]).size()) * 9.0f;
        platform.drawText(f, CHAR_NAMES[i],
                          {cx + (cardW - nameW) * 0.5f, cardY + 150.0f}, 16, nameColor);

        // "TAKEN" label for already-picked characters
        if (taken) {
            platform.drawText(f, "TAKEN", {cx + cardW * 0.5f - 30.0f, cardY + 190.0f}, 14,
                              Color{255, 80, 80, 200});
            // Show which player took it
            for (int p = 0; p < m_currentPicker; ++p) {
                if (m_picks[static_cast<size_t>(p)] == i) {
                    std::string takenBy = "P" + std::to_string(p + 1);
                    platform.drawText(f, takenBy, {cx + cardW * 0.5f - 10.0f, cardY + 210.0f}, 12,
                                      CHAR_COLORS[static_cast<size_t>(p)]);
                }
            }
        }

        // Selection indicator arrow
        if (selected && !taken) {
            float arrowBob = std::sin(m_elapsed * 5.0f) * 3.0f;
            platform.drawText(f, "v", {cx + cardW * 0.5f - 5.0f, cardY - 20.0f + arrowBob}, 20,
                              CHAR_COLORS[static_cast<size_t>(i)]);
        }
    }

    // Show already-confirmed picks at bottom
    if (m_currentPicker > 0) {
        float infoY = cardY + cardH + 30.0f;
        platform.drawText(f, "SELECTED:", {startX, infoY}, 14, Color{255, 255, 255, 160});
        for (int p = 0; p < m_currentPicker; ++p) {
            int charIdx = m_picks[static_cast<size_t>(p)];
            if (charIdx >= 0 && charIdx < 4) {
                std::string info = "P" + std::to_string(p + 1) + ": " + CHAR_NAMES[charIdx];
                platform.drawText(f, info,
                                  {startX + static_cast<float>(p) * 260.0f + 120.0f, infoY}, 14,
                                  CHAR_COLORS[static_cast<size_t>(charIdx)]);
            }
        }
    }
}

bool CharacterSelectScene::isTaken(int charIdx) const {
    for (int p = 0; p < m_currentPicker; ++p) {
        if (m_picks[static_cast<size_t>(p)] == charIdx) return true;
    }
    return false;
}

void CharacterSelectScene::confirmPick() {
    if (isTaken(m_cursor)) {
        // Can't pick a taken character — play error sound or just ignore
        return;
    }

    AudioManager::instance().playSound("menu_confirm");
    m_picks[static_cast<size_t>(m_currentPicker)] = m_cursor;
    m_currentPicker++;

    if (m_currentPicker >= m_numPickers) {
        finishSelection();
    } else {
        // Move cursor to next available character
        for (int i = 0; i < 4; ++i) {
            if (!isTaken(i)) {
                m_cursor = i;
                break;
            }
        }
    }
}

void CharacterSelectScene::finishSelection() {
    // Apply character assignments to GameState defaults via LevelSelectScene
    // We store picks on the Game object so GameScene can read them.
    // For simplicity, we use a static array that GameScene reads on init.
    auto& charPicks = m_game.characterPicks();
    for (int i = 0; i < 4; ++i) {
        if (m_picks[static_cast<size_t>(i)] >= 0) {
            charPicks[static_cast<size_t>(i)] = m_picks[static_cast<size_t>(i)];
        } else {
            // Auto-assign remaining players sequentially, skipping taken
            int assign = i; // default
            for (int c = 0; c < 4; ++c) {
                bool taken = false;
                for (int p = 0; p < 4; ++p) {
                    if (charPicks[static_cast<size_t>(p)] == c) { taken = true; break; }
                }
                if (!taken) { assign = c; break; }
            }
            charPicks[static_cast<size_t>(i)] = assign;
        }
    }

    m_game.scenes().replace(std::make_unique<LevelSelectScene>(m_game, m_mode));
}
