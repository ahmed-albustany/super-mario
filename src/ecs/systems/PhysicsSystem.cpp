#include "ecs/systems/PhysicsSystem.hpp"
#include "ecs/Components.hpp"
#include "core/GameConfig.hpp"
#include "utils/Math.hpp"

void PhysicsSystem::update(entt::registry& reg, float dt) {
    // Apply gravity to all entities that have VelocityComponent + GravityComponent
    auto view = reg.view<VelocityComponent, GravityComponent>();

    for (auto entity : view) {
        auto& vel   = view.get<VelocityComponent>(entity);
        auto& grav  = view.get<GravityComponent>(entity);

        // Check if player is wall-sliding — use reduced gravity
        float gravity = Config::GRAVITY * grav.multiplier;

        if (reg.all_of<PlayerComponent>(entity)) {
            const auto& player = reg.get<PlayerComponent>(entity);

            // No gravity during dash
            if (player.state == PlayerState::Dashing) {
                continue;
            }

            // Reduced gravity while wall sliding
            if (player.state == PlayerState::WallSliding) {
                gravity = Config::WALL_SLIDE_GRAVITY;
            }
        }

        vel.velocity.y += gravity * dt;

        // Cap at terminal velocity (downward only)
        if (vel.velocity.y > Config::TERMINAL_VELOCITY) {
            vel.velocity.y = Config::TERMINAL_VELOCITY;
        }

        // Cap upward velocity too (prevent extreme launches)
        if (vel.velocity.y < -Config::TERMINAL_VELOCITY) {
            vel.velocity.y = -Config::TERMINAL_VELOCITY;
        }

        // Cap horizontal velocity
        float maxHoriz = Config::PLAYER_DASH_SPEED * 1.5f;
        vel.velocity.x = Math::clamp(vel.velocity.x, -maxHoriz, maxHoriz);
    }

    // Update projectile lifetimes
    auto projView = reg.view<ProjectileComponent>();
    for (auto entity : projView) {
        auto& proj = projView.get<ProjectileComponent>(entity);
        proj.lifetime -= dt;
        if (proj.lifetime <= 0.0f) {
            reg.emplace_or_replace<DestroyFlag>(entity);
        }
    }
}
