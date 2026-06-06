#pragma once

#include <entt/entt.hpp>
#include <vector>

class IPlatform;

/// @brief Z-order sorted sprite rendering with optional debug AABB overlay (F1 toggle).
class RenderSystem {
public:
    void update(entt::registry& reg, IPlatform& platform);

private:
    struct RenderEntry {
        entt::entity entity;
        int zOrder;
    };

    std::vector<RenderEntry> m_entries;  // reused each frame to avoid allocation
    bool m_debugDraw = false;
    bool m_debugKeyWasPressed = false;
};
