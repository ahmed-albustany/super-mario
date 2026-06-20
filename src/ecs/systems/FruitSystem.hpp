#pragma once

#include <entt/entt.hpp>

class EventBus;
struct GameState;

/// @brief Handles fruit collection, scoring per player, extra life at 10000,
///        VS mode scoring, and fruit animation.
class FruitSystem {
public:
    void update(entt::registry& reg, float dt, EventBus& events);
};
