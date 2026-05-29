#pragma once

#include <entt/entt.hpp>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "utils/Math.hpp"

class EventBus;

/// @brief Broad-phase spatial grid + narrow-phase AABB collision detection and response.
///        Sets grounded/wall flags on PlayerComponent via proximity probes.
///        Handles entity-entity interactions (stomp, hurt, pickup, goal).
class CollisionSystem {
public:
    void update(entt::registry& reg, float dt, EventBus& events);

private:
    // ---- Spatial hash grid for broad phase ----
    static constexpr int CELL_SIZE = 64;

    static int64_t cellKey(int cx, int cy) {
        return (static_cast<int64_t>(cx) << 32) | static_cast<uint32_t>(cy);
    }

    void rebuildGrid(entt::registry& reg);
    void resolveStaticCollisions(entt::registry& reg);
    void resolveDynamicPairs(entt::registry& reg, EventBus& events);
    void updatePlayerProbes(entt::registry& reg);

    std::unordered_map<int64_t, std::vector<entt::entity>> m_grid;
    std::vector<std::pair<entt::entity, entt::entity>> m_pairs;
};
