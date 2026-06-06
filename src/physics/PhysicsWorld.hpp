#pragma once

#include "core/GameConfig.hpp"

/// @brief World-level physics configuration.
///        Holds gravity, friction, and velocity caps that can be overridden per level.
///        Used by PhysicsSystem as a config source instead of reading Config:: directly.
struct PhysicsWorld {
    float gravity          = Config::GRAVITY;
    float terminalVelocity = Config::TERMINAL_VELOCITY;
    float groundFriction   = 0.85f;   ///< Multiplied to horizontal velocity each frame on ground
    float airFriction      = 0.98f;   ///< Multiplied to horizontal velocity each frame in air
    float iceMultiplier    = 1.0f;    ///< Future: <1 = slippery, >1 = sticky

    /// @brief Reset to default Config values.
    void reset();

    /// @brief Override gravity from level data (if > 0).
    void loadFromLevel(float levelGravity);
};
