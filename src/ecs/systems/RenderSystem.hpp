#pragma once

#include <entt/entt.hpp>

class IPlatform;

/// @brief Z-order sorted sprite rendering with optional debug AABB overlay (F1 toggle).
class RenderSystem {
public:
    void update(entt::registry& reg, IPlatform& platform);

private:
    bool m_debugDraw = false;
    bool m_debugKeyWasPressed = false;
};
