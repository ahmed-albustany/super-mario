#pragma once

#include <entt/entt.hpp>

class EventBus;

/// @brief Applies active power-up effects, counts down timers, removes expired buffs.
///        Publishes PowerUpActivatedEvent on first application.
class PowerUpSystem {
public:
    void update(entt::registry& reg, float dt, EventBus& events);
};
