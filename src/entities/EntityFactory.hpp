#pragma once

#include "ecs/Components.hpp"
#include "utils/Math.hpp"

#include <entt/entt.hpp>
#include <string>
#include <vector>

/// @brief Static factory — creates fully-assembled ECS entities.
///        Every entity gets exactly the components it needs, no more, no less.
class EntityFactory {
public:
    EntityFactory() = delete;

    // ---- Player (Pixel Adventure characters) ----
    [[nodiscard]] static entt::entity createPlayer(entt::registry& registry, Vec2f spawnPos,
                                                    int playerIndex = 0);

    // ---- Fruits ----
    static entt::entity createFruit(entt::registry& registry, Vec2f pos,
                                     FruitType type, int value);
    static entt::entity createFruitByName(entt::registry& registry, Vec2f pos,
                                           const std::string& typeName);

    // ---- Traps ----
    static entt::entity createTrap(entt::registry& registry, Vec2f pos,
                                    TrapType type, float speed = 1.0f);
    static entt::entity createMovingPlatform(entt::registry& registry, Vec2f pos,
                                              const std::vector<Vec2f>& path, float speed);
    static entt::entity createFallingPlatform(entt::registry& registry, Vec2f pos);
    static entt::entity createSpikedBall(entt::registry& registry, Vec2f anchorPos,
                                          float chainLength);
    static entt::entity createFan(entt::registry& registry, Vec2f pos, float strength);
    static entt::entity createArrow(entt::registry& registry, Vec2f pos,
                                     const std::string& direction);
    static entt::entity createFire(entt::registry& registry, Vec2f pos,
                                    float onTime, float offTime);

    // ---- Boxes ----
    static entt::entity createBox(entt::registry& registry, Vec2f pos,
                                   const std::string& boxType, int hits);

    // ---- Checkpoints & Goals ----
    static entt::entity createCheckpoint(entt::registry& registry, Vec2f pos);
    static entt::entity createTrophy(entt::registry& registry, Vec2f pos);
    static entt::entity createStartSign(entt::registry& registry, Vec2f pos);

    // ---- Effects ----
    static entt::entity createParticleEffect(
        entt::registry& registry, Vec2f pos,
        ParticleEmitterComponent::Effect type);

    static entt::entity createFloatingText(
        entt::registry& registry, Vec2f pos,
        const std::string& text, Color color = Color{255, 255, 255, 255});

    // ---- Legacy compatibility ----
    [[nodiscard]] static entt::entity createGoomba(entt::registry& registry, Vec2f pos,
                                                    float patrolLeft, float patrolRight);
    [[nodiscard]] static entt::entity createKoopa(entt::registry& registry, Vec2f pos,
                                                   float patrolLeft, float patrolRight);
    [[nodiscard]] static entt::entity createPiranhaPlant(entt::registry& registry, Vec2f pos);
    [[nodiscard]] static entt::entity createBowser(entt::registry& registry, Vec2f pos,
                                                    float patrolLeft, float patrolRight);
    static entt::entity createCoin(entt::registry& registry, Vec2f pos);
    static entt::entity createMushroom(entt::registry& registry, Vec2f pos, bool fromBlock = false);
    static entt::entity createFireFlower(entt::registry& registry, Vec2f pos, bool fromBlock = false);
    static entt::entity createStar(entt::registry& registry, Vec2f pos, bool fromBlock = false);
    static entt::entity createOneUp(entt::registry& registry, Vec2f pos, bool fromBlock = false);
    [[nodiscard]] static entt::entity createQuestionBlock(entt::registry& registry, Vec2f pos,
                                                           CollectibleType contents);
    [[nodiscard]] static entt::entity createPipe(entt::registry& registry, Vec2f pos,
                                                   bool enterable, Vec2f destination);
    [[nodiscard]] static entt::entity createFlagPole(entt::registry& registry, Vec2f pos,
                                                      float height);
    [[nodiscard]] static entt::entity createGoal(entt::registry& registry, Vec2f pos);
    [[nodiscard]] static entt::entity createProjectile(entt::registry& registry, Vec2f pos,
                                                        Vec2f direction, entt::entity owner);

    // Legacy aliases
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
    [[nodiscard]] static entt::entity createGemShard(entt::registry& registry, Vec2f pos) {
        return createCoin(registry, pos);
    }
    [[nodiscard]] static entt::entity createPowerCrystal(entt::registry& registry, Vec2f pos) {
        return createStar(registry, pos);
    }

private:
    static void attachEnemyBase(entt::registry& registry, entt::entity entity,
                                Vec2f pos, EnemyType type, int hp,
                                float patrolLeft, float patrolRight,
                                int facing);
};
