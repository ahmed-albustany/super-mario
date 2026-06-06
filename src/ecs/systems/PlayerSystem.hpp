#pragma once

#include <entt/entt.hpp>

class InputManager;
class EventBus;

/// @brief Mario-style player state machine driven by input.
///        States: Idle, Running, Jumping, Falling, Skidding, Growing,
///        Shrinking, FlagPole, EnteringPipe, Hurt, Dead.
///        Implements run speed, variable jump height, fireball shooting.
class PlayerSystem {
public:
    void update(entt::registry& reg, float dt,
                const InputManager& input, EventBus& events);
};
