#pragma once

#include <entt/entt.hpp>

/// @brief Applies velocity to transform position each frame.
class MovementSystem {
public:
    void update(entt::registry& reg, float dt);
};
