#pragma once

#include <entt/entt.hpp>

/// @brief Per-type enemy AI logic.
///        Walker: edge-to-edge patrol.  Jumper: patrol + periodic bounce.
///        Shooter: sight-cone detection + fire projectile on cooldown.
///        Guardian: armored patrol, first hit removes armor, second kills.
class EnemyAISystem {
public:
    void update(entt::registry& reg, float dt);
};
