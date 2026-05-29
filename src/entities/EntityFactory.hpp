#pragma once

#include "ecs/Components.hpp"
#include "utils/Math.hpp"

#include <entt/entt.hpp>

/// @brief Static factory — creates fully-assembled ECS entities.
///        Every entity gets exactly the components it needs, no more, no less.
///        No game logic here — that lives in the systems.
class EntityFactory {
public:
    EntityFactory() = delete;

    // ---- Player ----
    [[nodiscard]] static entt::entity createPlayer(entt::registry& registry, Vec2f spawnPos);

    // ---- Enemies ----
    [[nodiscard]] static entt::entity createWalker(entt::registry& registry, Vec2f pos,
                                                    float patrolLeft, float patrolRight);
    [[nodiscard]] static entt::entity createJumper(entt::registry& registry, Vec2f pos,
                                                    float patrolLeft, float patrolRight);
    [[nodiscard]] static entt::entity createShooter(entt::registry& registry, Vec2f pos,
                                                     bool facingLeft);
    [[nodiscard]] static entt::entity createGuardian(entt::registry& registry, Vec2f pos,
                                                      float patrolLeft, float patrolRight);

    // ---- Collectibles ----
    [[nodiscard]] static entt::entity createCoin(entt::registry& registry, Vec2f pos);
    [[nodiscard]] static entt::entity createGemShard(entt::registry& registry, Vec2f pos);
    [[nodiscard]] static entt::entity createPowerCrystal(entt::registry& registry, Vec2f pos);

    // ---- Projectiles ----
    [[nodiscard]] static entt::entity createProjectile(entt::registry& registry, Vec2f pos,
                                                        Vec2f direction, entt::entity owner);

    // ---- Effects ----
    [[nodiscard]] static entt::entity createParticleEffect(
        entt::registry& registry, Vec2f pos,
        ParticleEmitterComponent::Effect type);

    // ---- World objects ----
    [[nodiscard]] static entt::entity createGoal(entt::registry& registry, Vec2f pos);

private:
    /// @brief Shared helper to attach base enemy components.
    static void attachEnemyBase(entt::registry& registry, entt::entity entity,
                                Vec2f pos, EnemyType type, int hp,
                                float patrolLeft, float patrolRight,
                                int facing);
};
