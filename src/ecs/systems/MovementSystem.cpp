#include "ecs/systems/MovementSystem.hpp"
#include "ecs/Components.hpp"

void MovementSystem::update(entt::registry& reg, float dt) {
    auto view = reg.view<TransformComponent, VelocityComponent>();

    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        const auto& vel = view.get<VelocityComponent>(entity);

        transform.position.x += vel.velocity.x * dt;
        transform.position.y += vel.velocity.y * dt;
    }

    // Move projectiles along their direction
    auto projView = reg.view<TransformComponent, ProjectileComponent>();
    for (auto entity : projView) {
        auto& transform = projView.get<TransformComponent>(entity);
        const auto& proj = projView.get<ProjectileComponent>(entity);

        // Projectile uses its own speed + direction rather than VelocityComponent
        // (some projectiles may not have VelocityComponent)
        if (!reg.all_of<VelocityComponent>(entity)) {
            transform.position.x += proj.direction.x * proj.speed * dt;
            transform.position.y += proj.direction.y * proj.speed * dt;
        }
    }
}
