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
    [[nodiscard]] static entt::entity createPlayer(entt::registry& registry, Vec2f spawnPos,
                                                    int playerIndex = 0);

    // ---- Enemies (Mario style) ----
    [[nodiscard]] static entt::entity createGoomba(entt::registry& registry, Vec2f pos,
                                                    float patrolLeft, float patrolRight);
    [[nodiscard]] static entt::entity createKoopa(entt::registry& registry, Vec2f pos,
                                                   float patrolLeft, float patrolRight);
    [[nodiscard]] static entt::entity createPiranhaPlant(entt::registry& registry, Vec2f pos);
    [[nodiscard]] static entt::entity createBowser(entt::registry& registry, Vec2f pos,
                                                    float patrolLeft, float patrolRight);

    // ---- Collectibles ----
    static entt::entity createCoin(entt::registry& registry, Vec2f pos);
    static entt::entity createMushroom(entt::registry& registry, Vec2f pos,
                                       bool fromBlock = false);
    static entt::entity createFireFlower(entt::registry& registry, Vec2f pos,
                                         bool fromBlock = false);
    static entt::entity createStar(entt::registry& registry, Vec2f pos,
                                    bool fromBlock = false);
    static entt::entity createOneUp(entt::registry& registry, Vec2f pos,
                                     bool fromBlock = false);

    // ---- World objects ----
    [[nodiscard]] static entt::entity createQuestionBlock(entt::registry& registry, Vec2f pos,
                                                           CollectibleType contents);
    [[nodiscard]] static entt::entity createPipe(entt::registry& registry, Vec2f pos,
                                                   bool enterable, Vec2f destination);
    [[nodiscard]] static entt::entity createFlagPole(entt::registry& registry, Vec2f pos,
                                                      float height);
    [[nodiscard]] static entt::entity createGoal(entt::registry& registry, Vec2f pos);

    // ---- Projectiles ----
    [[nodiscard]] static entt::entity createProjectile(entt::registry& registry, Vec2f pos,
                                                        Vec2f direction, entt::entity owner);

    // ---- Effects (fire-and-forget, return value may be ignored) ----
    static entt::entity createParticleEffect(
        entt::registry& registry, Vec2f pos,
        ParticleEmitterComponent::Effect type);

    static entt::entity createFloatingText(
        entt::registry& registry, Vec2f pos,
        const std::string& text, Color color = Color{255, 255, 255, 255});

    // ---- Legacy aliases for backward compatibility ----
    [[nodiscard]] static entt::entity createWalker(entt::registry& registry, Vec2f pos,
                                                    float patrolLeft, float patrolRight) {
        return createGoomba(registry, pos, patrolLeft, patrolRight);
    }
    [[nodiscard]] static entt::entity createJumper(entt::registry& registry, Vec2f pos,
                                                    float patrolLeft, float patrolRight) {
        return createKoopa(registry, pos, patrolLeft, patrolRight);
    }
    [[nodiscard]] static entt::entity createShooter(entt::registry& registry, Vec2f pos,
                                                     bool /*facingLeft*/) {
        return createPiranhaPlant(registry, pos);
    }
    [[nodiscard]] static entt::entity createGuardian(entt::registry& registry, Vec2f pos,
                                                      float patrolLeft, float patrolRight) {
        return createBowser(registry, pos, patrolLeft, patrolRight);
    }

    // Legacy collectible aliases
    [[nodiscard]] static entt::entity createGemShard(entt::registry& registry, Vec2f pos) {
        return createCoin(registry, pos);
    }
    [[nodiscard]] static entt::entity createPowerCrystal(entt::registry& registry, Vec2f pos) {
        return createStar(registry, pos);
    }

private:
    /// @brief Shared helper to attach base enemy components.
    static void attachEnemyBase(entt::registry& registry, entt::entity entity,
                                Vec2f pos, EnemyType type, int hp,
                                float patrolLeft, float patrolRight,
                                int facing);
};
