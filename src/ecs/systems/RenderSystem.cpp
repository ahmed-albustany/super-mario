#include "ecs/systems/RenderSystem.hpp"
#include "ecs/Components.hpp"
#include "platform/IPlatform.hpp"

#include <algorithm>
#include <vector>

void RenderSystem::update(entt::registry& reg, IPlatform& platform) {
    // ---- Toggle debug draw with F1 (KeyCode::Debug) ----
    bool debugKeyDown = platform.isKeyPressed(KeyCode::Debug);
    if (debugKeyDown && !m_debugKeyWasPressed) {
        m_debugDraw = !m_debugDraw;
    }
    m_debugKeyWasPressed = debugKeyDown;

    // ---- Collect and sort renderable entities by z-order ----
    struct RenderEntry {
        entt::entity entity;
        int zOrder;
    };

    std::vector<RenderEntry> entries;
    entries.reserve(128);

    auto view = reg.view<TransformComponent, SpriteComponent>();
    for (auto entity : view) {
        const auto& sprite = view.get<SpriteComponent>(entity);
        if (!sprite.visible) continue;
        entries.push_back({entity, sprite.zOrder});
    }

    // Stable sort by z-order (lower z draws first = behind)
    std::stable_sort(entries.begin(), entries.end(),
        [](const RenderEntry& a, const RenderEntry& b) {
            return a.zOrder < b.zOrder;
        });

    // ---- Draw sprites ----
    for (const auto& entry : entries) {
        const auto& transform = reg.get<TransformComponent>(entry.entity);
        const auto& sprite    = reg.get<SpriteComponent>(entry.entity);

        if (!sprite.texture.valid()) continue;

        platform.drawSprite(
            sprite.texture,
            sprite.srcRect,
            transform.position,
            transform.scale,
            transform.rotation,
            sprite.flipX,
            sprite.tint
        );
    }

    // ---- Debug AABB overlay ----
    if (m_debugDraw) {
        auto colliderView = reg.view<TransformComponent, ColliderComponent>();
        for (auto entity : colliderView) {
            const auto& transform = colliderView.get<TransformComponent>(entity);
            const auto& collider  = colliderView.get<ColliderComponent>(entity);

            Rect aabb = collider.toRect(transform.position);

            Color outlineColor;
            if (collider.isStatic) {
                outlineColor = Color{100, 100, 255, 180};   // blue for statics
            } else if (collider.isTrigger) {
                outlineColor = Color{255, 255, 0, 180};     // yellow for triggers
            } else if (reg.all_of<PlayerComponent>(entity)) {
                outlineColor = Color{0, 255, 0, 180};       // green for player
            } else if (reg.all_of<EnemyComponent>(entity)) {
                outlineColor = Color{255, 0, 0, 180};       // red for enemies
            } else {
                outlineColor = Color{255, 255, 255, 120};   // white for everything else
            }

            platform.drawRect(aabb, Color::Transparent(), outlineColor, 1.0f);
        }

        // Draw player ground/wall probes
        auto playerView = reg.view<PlayerComponent, TransformComponent, ColliderComponent>();
        for (auto pEnt : playerView) {
            const auto& pt = playerView.get<TransformComponent>(pEnt);
            const auto& pc = playerView.get<ColliderComponent>(pEnt);
            const auto& pp = playerView.get<PlayerComponent>(pEnt);

            Rect pRect = pc.toRect(pt.position);

            // Ground probe
            Rect gp = {pRect.x + 1.0f, pRect.bottom(), pRect.w - 2.0f, 2.0f};
            Color gpColor = pp.isGrounded ? Color{0, 255, 0, 200} : Color{255, 0, 0, 200};
            platform.drawRect(gp, gpColor, Color::Transparent(), 0.0f);

            // Wall probes
            Rect lp = {pRect.x - 2.0f, pRect.y + 2.0f, 2.0f, pRect.h - 4.0f};
            Rect rp = {pRect.right(),   pRect.y + 2.0f, 2.0f, pRect.h - 4.0f};
            Color lpColor = pp.isTouchingWallLeft  ? Color{0, 255, 0, 200} : Color{255, 100, 0, 120};
            Color rpColor = pp.isTouchingWallRight ? Color{0, 255, 0, 200} : Color{255, 100, 0, 120};
            platform.drawRect(lp, lpColor, Color::Transparent(), 0.0f);
            platform.drawRect(rp, rpColor, Color::Transparent(), 0.0f);
        }
    }
}
