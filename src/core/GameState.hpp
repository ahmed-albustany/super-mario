#pragma once

#include <memory>

/// @brief Shared gameplay state — owned by GameScene, safely shared with HUDScene.
///        Lives on the heap via shared_ptr so HUDScene never holds dangling references.
struct PlayerGameState {
    int   score = 0;
    int   lives = 3;
    int   coins = 0;
};

struct GameState {
    PlayerGameState p1;
    PlayerGameState p2;
    float levelTimer = 300.0f;
    bool  levelWon   = false;
    bool  gameOver   = false;
    int   numPlayers = 1;         ///< 1 or 2
    bool  coopMode   = false;     ///< true = co-op, false = alternating
    int   currentPlayer = 0;      ///< 0 = P1's turn, 1 = P2's turn (alternating mode)

    PlayerGameState& current() {
        return (currentPlayer == 0) ? p1 : p2;
    }

    const PlayerGameState& current() const {
        return (currentPlayer == 0) ? p1 : p2;
    }
};

using GameStatePtr = std::shared_ptr<GameState>;
