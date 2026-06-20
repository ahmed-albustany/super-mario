#pragma once

#include <entt/entt.hpp>

class EventBus;

/// @brief Handles all trap behaviors: saw rotation, spike head AI, rock head fall,
///        fire toggle, arrow shooting, platform movement/falling, fan force,
///        spiked ball swing, trampoline bounce.
class TrapSystem {
public:
    void update(entt::registry& reg, float dt, EventBus& events);
};
