#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>
#include "utils/Math.hpp"

/// @brief Game mode determines how many players and how they interact.
enum class GameMode {
    Solo,       ///< 1 player
    Alt2P,      ///< 2 players alternating (switch on death)
    Coop2P,     ///< 2 players simultaneously on screen
    Coop4P,     ///< 4 players simultaneously on screen
    VS4P        ///< 4 players competing for fruits
};

/// @brief Character types matching Pixel Adventure sprite sets.
enum class CharacterType {
    MaskDude,     ///< Player 1 default
    NinjaFrog,    ///< Player 2 default
    PinkMan,      ///< Player 3 default
    VirtualGuy    ///< Player 4 default
};

/// @brief Per-player state tracked across the game session.
struct PlayerGameState {
    int   score           = 0;
    int   lives           = 3;
    int   fruitsCollected = 0;
    int   playerIndex     = 0;       ///< 0-3
    CharacterType characterType = CharacterType::MaskDude;
    bool  isAlive         = true;
    Vec2f checkpointPos   = {0.0f, 0.0f}; ///< Last activated checkpoint (or spawn)

    // Legacy compatibility
    int   coins           = 0;
    int   powerState      = 0;
};

/// @brief Shared gameplay state — owned by GameScene, safely shared with HUDScene.
struct GameState {
    std::array<PlayerGameState, 4> players;
    float levelTimer = 300.0f;
    bool  levelWon   = false;
    bool  gameOver   = false;
    GameMode mode    = GameMode::Solo;
    int   currentPlayer = 0;      ///< Active player index (for alternating mode)

    // Level progression
    int   currentLevel = 0;
    std::string worldDisplay = "1-1";

    static inline const std::vector<std::string> LEVEL_PATHS = {
        "levels/level_01.json",
        "levels/level_02.json",
        "levels/level_03.json"
    };
    static inline const std::vector<std::string> WORLD_NAMES = {
        "1-1", "1-2", "1-3"
    };
    static constexpr int TOTAL_LEVELS = 3;

    GameState() {
        players[0].playerIndex = 0;
        players[0].characterType = CharacterType::MaskDude;
        players[1].playerIndex = 1;
        players[1].characterType = CharacterType::NinjaFrog;
        players[2].playerIndex = 2;
        players[2].characterType = CharacterType::PinkMan;
        players[3].playerIndex = 3;
        players[3].characterType = CharacterType::VirtualGuy;
    }

    bool hasNextLevel() const {
        return currentLevel + 1 < TOTAL_LEVELS;
    }

    /// @brief Get the number of active players based on game mode.
    int numActivePlayers() const {
        switch (mode) {
            case GameMode::Solo:   return 1;
            case GameMode::Alt2P:  return 2;
            case GameMode::Coop2P: return 2;
            case GameMode::Coop4P: return 4;
            case GameMode::VS4P:   return 4;
        }
        return 1;
    }

    /// @brief Is this mode simultaneous (multiple players on screen at once)?
    bool isSimultaneous() const {
        return mode == GameMode::Coop2P || mode == GameMode::Coop4P || mode == GameMode::VS4P;
    }

    /// @brief Is this VS mode?
    bool isVSMode() const {
        return mode == GameMode::VS4P;
    }

    PlayerGameState& current() {
        return players[static_cast<size_t>(currentPlayer)];
    }

    const PlayerGameState& current() const {
        return players[static_cast<size_t>(currentPlayer)];
    }

    // Legacy accessors for backward compatibility
    PlayerGameState& p1() { return players[0]; }
    const PlayerGameState& p1() const { return players[0]; }
    PlayerGameState& p2() { return players[1]; }
    const PlayerGameState& p2() const { return players[1]; }

    int numPlayers() const { return numActivePlayers(); }
    bool coopMode() const { return isSimultaneous(); }
};

using GameStatePtr = std::shared_ptr<GameState>;
