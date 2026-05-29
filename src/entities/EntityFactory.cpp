#include "entities/EntityFactory.hpp"
#include "core/GameConfig.hpp"

// =============================================================================
// Player
// =============================================================================

entt::entity EntityFactory::createPlayer(entt::registry& registry, Vec2f spawnPos) {
    auto entity = registry.create();

    registry.emplace<TransformComponent>(entity, TransformComponent{spawnPos});
    registry.emplace<VelocityComponent>(entity);
    registry.emplace<GravityComponent>(entity);
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {4.0f, 0.0f}, {24.0f, 32.0f}, false, false
    });
    registry.emplace<SpriteComponent>(entity);
    registry.emplace<AnimationComponent>(entity);
    registry.emplace<PlayerComponent>(entity);
    registry.emplace<HealthComponent>(entity, HealthComponent{
        Config::PLAYER_MAX_HP, Config::PLAYER_MAX_HP, 0, false
    });
    registry.emplace<TagComponent>(entity, TagComponent{"player"});

    return entity;
}

// =============================================================================
// Enemies — shared base
// =============================================================================

void EntityFactory::attachEnemyBase(entt::registry& registry, entt::entity entity,
                                    Vec2f pos, EnemyType type, int hp,
                                    float patrolLeft, float patrolRight,
                                    int facing) {
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<VelocityComponent>(entity);
    registry.emplace<GravityComponent>(entity);
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {4.0f, 4.0f}, {24.0f, 28.0f}, false, false
    });
    registry.emplace<SpriteComponent>(entity);
    registry.emplace<AnimationComponent>(entity);
    registry.emplace<HealthComponent>(entity, HealthComponent{hp, hp, 0, false});

    EnemyComponent ec{};
    ec.type        = type;
    ec.state       = EnemyState::Patrol;
    ec.patrolLeft  = patrolLeft;
    ec.patrolRight = patrolRight;
    ec.facing      = facing;
    ec.isAlerted   = false;
    ec.isArmored   = (type == EnemyType::Guardian);
    registry.emplace<EnemyComponent>(entity, ec);
}

// =============================================================================
// Walker
// =============================================================================

entt::entity EntityFactory::createWalker(entt::registry& registry, Vec2f pos,
                                          float patrolLeft, float patrolRight) {
    auto entity = registry.create();
    attachEnemyBase(registry, entity, pos, EnemyType::Walker, 1,
                    patrolLeft, patrolRight, 1);
    registry.emplace<TagComponent>(entity, TagComponent{"walker"});
    return entity;
}

// =============================================================================
// Jumper
// =============================================================================

entt::entity EntityFactory::createJumper(entt::registry& registry, Vec2f pos,
                                          float patrolLeft, float patrolRight) {
    auto entity = registry.create();
    attachEnemyBase(registry, entity, pos, EnemyType::Jumper, 1,
                    patrolLeft, patrolRight, 1);

    // Jumper-specific tuning
    auto& ec         = registry.get<EnemyComponent>(entity);
    ec.bounceForce   = -500.0f;
    ec.bounceTimer   = 1.5f;

    registry.emplace<TagComponent>(entity, TagComponent{"jumper"});
    return entity;
}

// =============================================================================
// Shooter
// =============================================================================

entt::entity EntityFactory::createShooter(entt::registry& registry, Vec2f pos,
                                           bool facingLeft) {
    auto entity = registry.create();
    int facing = facingLeft ? -1 : 1;
    // Shooters are mostly stationary — patrol range is tight around their position
    attachEnemyBase(registry, entity, pos, EnemyType::Shooter, 1,
                    pos.x - 16.0f, pos.x + 16.0f, facing);

    // Shooter-specific tuning
    auto& ec          = registry.get<EnemyComponent>(entity);
    ec.shootCooldown  = 0.0f;
    ec.shootInterval  = 2.0f;

    registry.emplace<TagComponent>(entity, TagComponent{"shooter"});
    return entity;
}

// =============================================================================
// Guardian
// =============================================================================

entt::entity EntityFactory::createGuardian(entt::registry& registry, Vec2f pos,
                                            float patrolLeft, float patrolRight) {
    auto entity = registry.create();
    attachEnemyBase(registry, entity, pos, EnemyType::Guardian, 2,
                    patrolLeft, patrolRight, 1);
    registry.emplace<TagComponent>(entity, TagComponent{"guardian"});
    return entity;
}

// =============================================================================
// Collectibles
// =============================================================================

entt::entity EntityFactory::createCoin(entt::registry& registry, Vec2f pos) {
    auto entity = registry.create();

    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {4.0f, 4.0f}, {24.0f, 24.0f}, true, false
    });
    registry.emplace<SpriteComponent>(entity);
    registry.emplace<AnimationComponent>(entity);
    registry.emplace<CollectibleComponent>(entity, CollectibleComponent{
        CollectibleType::Coin, Config::COIN_VALUE, false
    });
    registry.emplace<TagComponent>(entity, TagComponent{"coin"});

    return entity;
}

entt::entity EntityFactory::createGemShard(entt::registry& registry, Vec2f pos) {
    auto entity = registry.create();

    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {4.0f, 4.0f}, {24.0f, 24.0f}, true, false
    });
    registry.emplace<SpriteComponent>(entity);
    registry.emplace<AnimationComponent>(entity);
    registry.emplace<CollectibleComponent>(entity, CollectibleComponent{
        CollectibleType::GemShard, Config::GEM_VALUE, false
    });
    registry.emplace<TagComponent>(entity, TagComponent{"gem_shard"});

    return entity;
}

entt::entity EntityFactory::createPowerCrystal(entt::registry& registry, Vec2f pos) {
    auto entity = registry.create();

    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {4.0f, 4.0f}, {24.0f, 24.0f}, true, false
    });
    registry.emplace<SpriteComponent>(entity);
    registry.emplace<AnimationComponent>(entity);
    registry.emplace<CollectibleComponent>(entity, CollectibleComponent{
        CollectibleType::PowerCrystal, 0, false
    });
    registry.emplace<TagComponent>(entity, TagComponent{"power_crystal"});

    return entity;
}

// =============================================================================
// Projectile
// =============================================================================

entt::entity EntityFactory::createProjectile(entt::registry& registry, Vec2f pos,
                                              Vec2f direction, entt::entity owner) {
    auto entity = registry.create();

    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<VelocityComponent>(entity, VelocityComponent{
        {direction.x * 300.0f, direction.y * 300.0f}
    });
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {2.0f, 2.0f}, {12.0f, 12.0f}, true, false
    });
    registry.emplace<SpriteComponent>(entity);
    registry.emplace<ProjectileComponent>(entity, ProjectileComponent{
        static_cast<uint32_t>(entt::to_integral(owner)),
        1,              // damage
        3.0f,           // lifetime
        300.0f,         // speed
        direction
    });
    registry.emplace<TagComponent>(entity, TagComponent{"projectile"});

    return entity;
}

// =============================================================================
// Particle effect
// =============================================================================

entt::entity EntityFactory::createParticleEffect(entt::registry& registry, Vec2f pos,
                                                  ParticleEmitterComponent::Effect type) {
    auto entity = registry.create();

    registry.emplace<TransformComponent>(entity, TransformComponent{pos});

    ParticleEmitterComponent emitter{};
    emitter.effect  = type;
    emitter.active  = true;
    emitter.elapsed = 0.0f;

    switch (type) {
        case ParticleEmitterComponent::Effect::DoubleJumpBurst:
            emitter.lifetime      = 0.3f;
            emitter.particleCount = 6;
            emitter.color         = Color{200, 220, 255, 200};
            break;
        case ParticleEmitterComponent::Effect::DashAfterimage:
            emitter.lifetime      = 0.25f;
            emitter.particleCount = 4;
            emitter.color         = Color{180, 140, 255, 180};
            break;
        case ParticleEmitterComponent::Effect::CoinSparkle:
            emitter.lifetime      = 0.4f;
            emitter.particleCount = 5;
            emitter.color         = Color{255, 220, 50, 230};
            break;
        case ParticleEmitterComponent::Effect::DestructionDebris:
            emitter.lifetime      = 0.6f;
            emitter.particleCount = 10;
            emitter.color         = Color{160, 140, 120, 220};
            break;
        case ParticleEmitterComponent::Effect::DeathPoof:
            emitter.lifetime      = 0.5f;
            emitter.particleCount = 8;
            emitter.color         = Color{255, 255, 255, 200};
            break;
    }

    registry.emplace<ParticleEmitterComponent>(entity, emitter);
    registry.emplace<TagComponent>(entity, TagComponent{"particle"});

    return entity;
}

// =============================================================================
// Goal (flagpole / exit portal)
// =============================================================================

entt::entity EntityFactory::createGoal(entt::registry& registry, Vec2f pos) {
    auto entity = registry.create();

    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {32.0f, 64.0f}, true, false
    });
    registry.emplace<SpriteComponent>(entity);
    registry.emplace<GoalComponent>(entity);
    registry.emplace<TagComponent>(entity, TagComponent{"goal"});

    return entity;
}
