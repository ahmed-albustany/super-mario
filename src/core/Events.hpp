#pragma once

#include <string>
#include "utils/Math.hpp"

/// @brief All game event types used with EventBus.

// =============================================================================
// Player events
// =============================================================================

struct PlayerDiedEvent {
    int livesRemaining = 0;
};

struct PlayerHurtEvent {
    int newHP = 0;
};

/// @brief Fired when player jumps (ground jump or double jump).
struct PlayerJumpEvent {
    int jumpNumber = 1;  ///< 1 = ground jump, 2 = double jump
};

/// @brief Fired when player initiates a dash.
struct PlayerDashEvent {};

/// @brief Fired when player wall-jumps.
struct PlayerWallJumpEvent {};

/// @brief Fired when player lands on ground after being airborne.
struct PlayerLandedEvent {};

// =============================================================================
// Collectible events
// =============================================================================

struct CoinCollectedEvent {
    int value = 0;
    Vec2f position;
};

struct GemCollectedEvent {
    int value = 0;
    Vec2f position;
};

// =============================================================================
// Enemy events
// =============================================================================

struct EnemyKilledEvent {
    std::string type;
    Vec2f position;
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
};

struct GameOverEvent {};

// =============================================================================
// Power-up events
// =============================================================================

struct PowerUpActivatedEvent {
    std::string type;
    float duration = 0.0f;
};

/// @brief Fired when a power-up expires.
struct PowerUpExpiredEvent {
    std::string type;
};
