#pragma once

#include <string>
#include "utils/Math.hpp"

/// @brief All game event types used with EventBus.

// =============================================================================
// Player events
// =============================================================================

struct PlayerDiedEvent {
    int livesRemaining = 0;
    int playerIndex    = 0;   ///< 0-3
};

struct PlayerHurtEvent {
    int newHP = 0;
    int playerIndex = 0;
};

struct PlayerJumpEvent {
    int jumpNumber = 1;       ///< 1 = first jump, 2 = double jump
    int playerIndex = 0;
};

struct PlayerLandedEvent {
    int playerIndex = 0;
};

struct PlayerPowerUpEvent {
    std::string powerType;
    int playerIndex = 0;
};

struct FireballEvent {
    Vec2f position;
    int playerIndex = 0;
};

// =============================================================================
// Fruit events (replaces coin)
// =============================================================================

struct FruitCollectedEvent {
    std::string fruitType;    ///< "cherry", "apple", etc.
    int value = 0;
    Vec2f position;
    int playerIndex = 0;
};

// =============================================================================
// Box events (replaces block)
// =============================================================================

struct BoxHitEvent {
    Vec2f position;
    int hitsRemaining = 0;
    int playerIndex = 0;
};

struct BoxBreakEvent {
    Vec2f position;
    std::string fruitSpawned; ///< fruit type spawned
    int playerIndex = 0;
};

// =============================================================================
// Trap events
// =============================================================================

struct TrapDeathEvent {
    std::string trapType;
    Vec2f position;
    int playerIndex = 0;
};

// =============================================================================
// Checkpoint / Level events
// =============================================================================

struct CheckpointActivatedEvent {
    Vec2f position;
    int playerIndex = 0;
};

struct LevelCompleteEvent {
    int score = 0;
    float timeRemaining = 0.0f;
    int playerIndex = 0;
};

struct GameOverEvent {
    int playerIndex = 0;
};

struct FlagPoleGrabbedEvent {
    float grabHeight = 0.0f;
    int playerIndex = 0;
};

// =============================================================================
// Power-up events
// =============================================================================

struct PowerUpActivatedEvent {
    std::string type;
    float duration = 0.0f;
    int playerIndex = 0;
};

struct PowerUpExpiredEvent {
    std::string type;
    int playerIndex = 0;
};

// =============================================================================
// Enemy events (kept for compatibility)
// =============================================================================

struct EnemyKilledEvent {
    std::string type;
    Vec2f position;
    int scoreValue = 0;
};

struct EnemyShootEvent {
    Vec2f position;
};

// =============================================================================
// Backward compatibility aliases
// =============================================================================

struct CoinCollectedEvent {
    int value = 0;
    Vec2f position;
    int playerIndex = 0;
};

using GemCollectedEvent = CoinCollectedEvent;

struct BlockHitEvent {
    Vec2f position;
    std::string contents;
    int playerIndex = 0;
};
