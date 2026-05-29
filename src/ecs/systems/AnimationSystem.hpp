#pragma once

#include <entt/entt.hpp>

/// @brief Advances sprite animation frames and maps entity state to clip name.
///        Handles direction flip via SpriteComponent.flipX.
class AnimationSystem {
public:
    void update(entt::registry& reg, float dt);
};
