#pragma once

#include <entt/entt.hpp>

/// @brief Applies gravity to velocity and caps at terminal velocity.
///        Handles wall-slide reduced gravity for players.
class PhysicsSystem {
public:
    void update(entt::registry& reg, float dt);
};
