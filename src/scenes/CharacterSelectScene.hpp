#pragma once

#include "scenes/IScene.hpp"
#include "core/GameState.hpp"
#include <array>
#include <string>

class Game;

/// @brief Character selection screen — each player picks a character.
///        Solo: 1 pick. Alt2P: 2 sequential picks. Coop2P: 2 sequential picks.
///        4P modes skip here (auto-assigned).
class CharacterSelectScene final : public IScene {
public:
    CharacterSelectScene(Game& game, GameMode mode);

    void onEnter() override;
    void onExit() override;
    void handleInput(const InputManager& input) override;
    void update(float dt) override;
    void render(IPlatform& platform) override;
    [[nodiscard]] std::string name() const override { return "CharacterSelectScene"; }

private:
    Game&    m_game;
    GameMode m_mode;
    float    m_elapsed = 0.0f;

    int m_currentPicker  = 0;   ///< Which player is currently picking (0 or 1)
    int m_numPickers     = 1;   ///< Total players that need to pick
    int m_cursor         = 0;   ///< Currently highlighted character (0-3)

    /// @brief Character assignments per player. -1 = not yet picked.
    std::array<int, 4> m_picks = {-1, -1, -1, -1};

    /// @brief Is this character already taken by another player?
    bool isTaken(int charIdx) const;

    void confirmPick();
    void finishSelection();
};
