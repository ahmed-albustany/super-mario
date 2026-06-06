#pragma once

#include <memory>
#include <string>
#include <vector>

/// @brief Shared gameplay state — owned by GameScene, safely shared with HUDScene.
///        Lives on the heap via shared_ptr so HUDScene never holds dangling references.
struct PlayerGameState {
    int   score       = 0;
    int   lives       = 3;
    int   coins       = 0;
    int   powerState  = 0;    ///< 0 = Small, 1 = Big, 2 = Fire (mirrors MarioPowerState)
    int   playerIndex = 0;    ///< 0 = P1 (Mario), 1 = P2 (Luigi)
};

struct GameState {
    PlayerGameState p1{0, 3, 0, 0, 0};
    PlayerGameState p2{0, 3, 0, 0, 1};
    float levelTimer = 300.0f;
    bool  levelWon   = false;
    bool  gameOver   = false;
    int   numPlayers = 1;         ///< 1 or 2
    bool  coopMode   = false;     ///< true = co-op, false = alternating
    int   currentPlayer = 0;      ///< 0 = P1's turn, 1 = P2's turn (alternating mode)

    // Level progression
    int   currentLevel = 0;       ///< Index into LEVEL_PATHS (0-based)
    std::string worldDisplay = "1-1"; ///< Display string for HUD

    static inline const std::vector<std::string> LEVEL_PATHS = {
        "levels/level_01.json",
        "levels/level_02.json",
        "levels/level_03.json"
    };
    static inline const std::vector<std::string> WORLD_NAMES = {
        "1-1", "1-2", "1-4"
    };
    static constexpr int TOTAL_LEVELS = 3;

    bool hasNextLevel() const {
        return currentLevel + 1 < TOTAL_LEVELS;
    }

    PlayerGameState& current() {
        return (currentPlayer == 0) ? p1 : p2;
    }

    const PlayerGameState& current() const {
        return (currentPlayer == 0) ? p1 : p2;
    }
};

using GameStatePtr = std::shared_ptr<GameState>;
