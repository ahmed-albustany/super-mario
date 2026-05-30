#include "entities/EntityFactory.hpp"
#include "core/GameConfig.hpp"
#include "core/ResourceManager.hpp"

namespace {

/// @brief Helper — look up a texture from ResourceManager, return handle (may be invalid).
TextureHandle tex(const std::string& key) {
    auto opt = ResourceManager::instance().getTexture(key);
    return opt.value_or(TextureHandle{0});
}

/// @brief Build a single-frame animation clip.
AnimationComponent::Clip makeClip(const std::string& name, const std::string& textureKey,
                                   const Rect& frame, float fps = 10.0f, bool loop = true) {
    AnimationComponent::Clip clip;
    clip.name    = name;
    clip.frames  = {frame};
    clip.fps     = fps;
    clip.loop    = loop;
    clip.texture = tex(textureKey);
    return clip;
}

// Standard source rects for whole-image sprites
constexpr Rect CHAR_RECT  = {0.0f, 0.0f, 24.0f, 24.0f}; // 24x24 character sprites
constexpr Rect ITEM_RECT  = {0.0f, 0.0f, 18.0f, 18.0f}; // 18x18 item sprites

} // anonymous namespace

// =============================================================================
// Player
// =============================================================================

entt::entity EntityFactory::createPlayer(entt::registry& registry, Vec2f spawnPos) {
    auto entity = registry.create();

    registry.emplace<TransformComponent>(entity, TransformComponent{spawnPos});
    registry.emplace<VelocityComponent>(entity);
    registry.emplace<GravityComponent>(entity);
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {24.0f, 24.0f}, false, false
    });

    // Sprite — start with idle texture
    SpriteComponent sprite;
    sprite.texture = tex("player_idle");
    sprite.srcRect = CHAR_RECT;
    sprite.zOrder  = 10; // player renders above most things
    registry.emplace<SpriteComponent>(entity, sprite);

    // Animation clips — one clip per player state, each referencing its own texture
    AnimationComponent anim;
    anim.clips = {
        makeClip("idle",        "player_idle",      CHAR_RECT, 1.0f, true),
        makeClip("run",         "player_run",       CHAR_RECT, 8.0f, true),
        makeClip("jump",        "player_jump",      CHAR_RECT, 1.0f, false),
        makeClip("double_jump", "player_jump",      CHAR_RECT, 1.0f, false),
        makeClip("fall",        "player_fall",      CHAR_RECT, 1.0f, true),
        makeClip("dash",        "player_dash",      CHAR_RECT, 1.0f, false),
        makeClip("wall_slide",  "player_wallslide", CHAR_RECT, 1.0f, true),
        makeClip("wall_jump",   "player_jump",      CHAR_RECT, 1.0f, false),
        makeClip("hurt",        "player_hurt",      CHAR_RECT, 1.0f, false),
        makeClip("dead",        "player_dead",      CHAR_RECT, 1.0f, false),
    };
    anim.currentClip = 0;
    registry.emplace<AnimationComponent>(entity, anim);

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
        {0.0f, 0.0f}, {24.0f, 24.0f}, false, false
    });

    // Determine texture key based on enemy type
    std::string texKey;
    switch (type) {
        case EnemyType::Walker:   texKey = "walker";   break;
        case EnemyType::Jumper:   texKey = "jumper";   break;
        case EnemyType::Shooter:  texKey = "shooter";  break;
        case EnemyType::Guardian: texKey = "guardian";  break;
    }

    SpriteComponent sprite;
    sprite.texture = tex(texKey);
    sprite.srcRect = CHAR_RECT;
    sprite.zOrder  = 5;
    registry.emplace<SpriteComponent>(entity, sprite);

    // Simple animation — all states use the same single-frame texture
    AnimationComponent anim;
    anim.clips = {
        makeClip("idle",   texKey, CHAR_RECT, 1.0f, true),
        makeClip("walk",   texKey, CHAR_RECT, 6.0f, true),
        makeClip("attack", texKey, CHAR_RECT, 1.0f, false),
        makeClip("hurt",   texKey, CHAR_RECT, 1.0f, false),
        makeClip("dead",   texKey, CHAR_RECT, 1.0f, false),
    };
    anim.currentClip = 0;
    registry.emplace<AnimationComponent>(entity, anim);

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
        {0.0f, 0.0f}, {18.0f, 18.0f}, true, false
    });

    SpriteComponent sprite;
    sprite.texture = tex("coin");
    sprite.srcRect = ITEM_RECT;
    sprite.zOrder  = 3;
    registry.emplace<SpriteComponent>(entity, sprite);

    AnimationComponent anim;
    anim.clips = {makeClip("idle", "coin", ITEM_RECT, 1.0f, true)};
    registry.emplace<AnimationComponent>(entity, anim);

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
        {0.0f, 0.0f}, {18.0f, 18.0f}, true, false
    });

    SpriteComponent sprite;
    sprite.texture = tex("gem_shard");
    sprite.srcRect = ITEM_RECT;
    sprite.zOrder  = 3;
    registry.emplace<SpriteComponent>(entity, sprite);

    AnimationComponent anim;
    anim.clips = {makeClip("idle", "gem_shard", ITEM_RECT, 1.0f, true)};
    registry.emplace<AnimationComponent>(entity, anim);

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
        {0.0f, 0.0f}, {18.0f, 18.0f}, true, false
    });

    SpriteComponent sprite;
    sprite.texture = tex("power_crystal");
    sprite.srcRect = ITEM_RECT;
    sprite.zOrder  = 3;
    registry.emplace<SpriteComponent>(entity, sprite);

    AnimationComponent anim;
    anim.clips = {makeClip("idle", "power_crystal", ITEM_RECT, 1.0f, true)};
    registry.emplace<AnimationComponent>(entity, anim);

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
        {3.0f, 3.0f}, {18.0f, 18.0f}, true, false
    });

    SpriteComponent sprite;
    sprite.texture = tex("projectile");
    sprite.srcRect = CHAR_RECT;
    sprite.zOrder  = 8;
    registry.emplace<SpriteComponent>(entity, sprite);

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
        {0.0f, 0.0f}, {18.0f, 18.0f}, true, false
    });

    SpriteComponent sprite;
    sprite.texture = tex("flagpole");
    sprite.srcRect = ITEM_RECT;
    sprite.zOrder  = 2;
    registry.emplace<SpriteComponent>(entity, sprite);

    registry.emplace<GoalComponent>(entity);
    registry.emplace<TagComponent>(entity, TagComponent{"goal"});

    return entity;
}
