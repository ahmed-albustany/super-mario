#pragma once

#include <entt/entt.hpp>

class InputManager;
class EventBus;

/// @brief Full player state machine driven by input.
///        States: Idle, Running, Jumping, DoubleJumping, Falling,
///        Dashing, WallSliding, WallJumping, Hurt, Dead.
///        Implements coyote time, jump buffering, dash cooldown.
class PlayerSystem {
public:
    void update(entt::registry& reg, float dt,
                const InputManager& input, EventBus& events);
};
