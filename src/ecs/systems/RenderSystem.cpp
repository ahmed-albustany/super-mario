#include "ecs/systems/RenderSystem.hpp"
#include "ecs/Components.hpp"
#include "core/ResourceManager.hpp"
#include "platform/IPlatform.hpp"

#include <algorithm>
#include <cmath>

void RenderSystem::update(entt::registry& reg, IPlatform& platform) {
    // ---- Toggle debug draw with F1 (KeyCode::Debug) ----
    bool debugKeyDown = platform.isKeyPressed(KeyCode::Debug);
    if (debugKeyDown && !m_debugKeyWasPressed) {
        m_debugDraw = !m_debugDraw;
    }
    m_debugKeyWasPressed = debugKeyDown;

    // ---- Collect and sort renderable entities by z-order ----
    m_entries.clear();

    auto view = reg.view<TransformComponent, SpriteComponent>();
    for (auto entity : view) {
        const auto& sprite = view.get<SpriteComponent>(entity);
        if (!sprite.visible) continue;
        m_entries.push_back({entity, sprite.zOrder});
    }

    // Stable sort by z-order (lower z draws first = behind)
    std::stable_sort(m_entries.begin(), m_entries.end(),
        [](const RenderEntry& a, const RenderEntry& b) {
            return a.zOrder < b.zOrder;
        });

    // ---- Draw sprites ----
    for (const auto& entry : m_entries) {
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

        // Draw player ground probe
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
        }
    }

    // ---- Particle effects (simple colored rects) ----
    {
        auto particleView = reg.view<ParticleEmitterComponent, TransformComponent>();
        for (auto entity : particleView) {
            const auto& pe  = particleView.get<ParticleEmitterComponent>(entity);
            const auto& pos = particleView.get<TransformComponent>(entity);
            if (!pe.active) continue;

            float progress = pe.elapsed / pe.lifetime;
            auto alpha = static_cast<uint8_t>((1.0f - progress) * static_cast<float>(pe.color.a));
            Color c = {pe.color.r, pe.color.g, pe.color.b, alpha};

            for (int i = 0; i < pe.particleCount; ++i) {
                // Spread particles in a circle expanding over time
                float angle = static_cast<float>(i) * 6.2832f / static_cast<float>(pe.particleCount);
                float radius = progress * 20.0f;
                float px = pos.position.x + std::cos(angle) * radius;
                float py = pos.position.y + std::sin(angle) * radius - progress * 15.0f;
                float sz = 3.0f * (1.0f - progress);
                platform.drawRect({px, py, sz, sz}, c);
            }
        }
    }

    // ---- Floating score text ----
    {
        auto font = ResourceManager::instance().getFont("main");
        if (font) {
            FontHandle f = *font;
            auto floatingTextView = reg.view<FloatingTextComponent, TransformComponent>();
            for (auto entity : floatingTextView) {
                const auto& ft  = floatingTextView.get<FloatingTextComponent>(entity);
                const auto& pos = floatingTextView.get<TransformComponent>(entity);
                float progress = ft.elapsed / ft.lifetime;
                auto alpha = static_cast<uint8_t>((1.0f - progress) * static_cast<float>(ft.color.a));
                Color c = {ft.color.r, ft.color.g, ft.color.b, alpha};
                platform.drawText(f, ft.text, pos.position, ft.fontSize, c);
            }
        }
    }
}
