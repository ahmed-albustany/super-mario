#pragma once

#include <string>
#include "utils/Math.hpp"

/// @brief All game event types used with EventBus.

// =============================================================================
// Player events
// =============================================================================

struct PlayerDiedEvent {
    int livesRemaining = 0;
    int playerIndex    = 0;   ///< 0 = P1, 1 = P2
};

struct PlayerHurtEvent {
    int newHP = 0;
    int playerIndex = 0;
};

/// @brief Fired when player jumps.
struct PlayerJumpEvent {
    int jumpNumber = 1;
    int playerIndex = 0;
};

/// @brief Fired when player lands on ground after being airborne.
struct PlayerLandedEvent {
    int playerIndex = 0;
};

/// @brief Fired when player's power state changes.
struct PlayerPowerUpEvent {
    std::string powerType; ///< "mushroom", "fire_flower", "star"
    int playerIndex = 0;
};

/// @brief Fired when player shoots a fireball.
struct FireballEvent {
    Vec2f position;
    int playerIndex = 0;
};

// =============================================================================
// Collectible events
// =============================================================================

struct CoinCollectedEvent {
    int value = 0;
    Vec2f position;
    int playerIndex = 0;
};

// =============================================================================
// Block events
// =============================================================================

/// @brief Fired when a question block or brick is hit from below.
struct BlockHitEvent {
    Vec2f position;
    std::string contents;  ///< "coin", "mushroom", "fire_flower", "star", "1up"
    int playerIndex = 0;
};

// =============================================================================
// Enemy events
// =============================================================================

struct EnemyKilledEvent {
    std::string type;
    Vec2f position;
    int scoreValue = 0;
};

/// @brief Fired when a shooter enemy fires a projectile.
struct EnemyShootEvent {
    Vec2f position;
};

// =============================================================================
// Level events
// =============================================================================

struct LevelCompleteEvent {
    int score = 0;
    float timeRemaining = 0.0f;
    int playerIndex = 0;
};

struct GameOverEvent {
    int playerIndex = 0;
};

/// @brief Fired when player grabs the flagpole.
struct FlagPoleGrabbedEvent {
    float grabHeight = 0.0f; ///< 0.0 = bottom, 1.0 = top (affects score)
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

/// @brief Fired when a power-up expires.
struct PowerUpExpiredEvent {
    std::string type;
    int playerIndex = 0;
};

// =============================================================================
// Backward compatibility aliases
// =============================================================================

using GemCollectedEvent = CoinCollectedEvent;
