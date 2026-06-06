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

// Standard source rects
constexpr Rect SMALL_RECT = {0.0f, 0.0f, 16.0f, 16.0f}; // Small Mario
constexpr Rect BIG_RECT   = {0.0f, 0.0f, 16.0f, 32.0f}; // Big/Fire Mario
constexpr Rect ENEMY_RECT = {0.0f, 0.0f, 16.0f, 16.0f}; // Goomba/Koopa
constexpr Rect ITEM_RECT  = {0.0f, 0.0f, 16.0f, 16.0f}; // Coins, items
constexpr Rect TILE_RECT  = {0.0f, 0.0f, 32.0f, 32.0f}; // Block tiles
constexpr Rect PIPE_RECT  = {0.0f, 0.0f, 64.0f, 64.0f}; // Pipe (2x2 tiles)
constexpr Rect BOWSER_RECT = {0.0f, 0.0f, 32.0f, 32.0f}; // Bowser

} // anonymous namespace

// =============================================================================
// Player
// =============================================================================

entt::entity EntityFactory::createPlayer(entt::registry& registry, Vec2f spawnPos,
                                          int playerIndex) {
    auto entity = registry.create();

    registry.emplace<TransformComponent>(entity, TransformComponent{spawnPos});
    registry.emplace<VelocityComponent>(entity);
    registry.emplace<GravityComponent>(entity);
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {14.0f, 16.0f}, false, false
    });

    // Sprite prefix: "mario_" for P1, "luigi_" for P2
    std::string prefix = (playerIndex == 0) ? "mario_" : "luigi_";

    SpriteComponent sprite;
    sprite.texture = tex(prefix + "small_idle");
    sprite.srcRect = SMALL_RECT;
    sprite.zOrder  = 10;
    registry.emplace<SpriteComponent>(entity, sprite);

    // Animation clips for all Mario states + power states
    AnimationComponent anim;
    // Small Mario clips
    anim.clips = {
        makeClip("idle",     prefix + "small_idle",    SMALL_RECT, 1.0f, true),
        makeClip("run",      prefix + "small_run",     SMALL_RECT, 10.0f, true),
        makeClip("jump",     prefix + "small_jump",    SMALL_RECT, 1.0f, false),
        makeClip("fall",     prefix + "small_jump",    SMALL_RECT, 1.0f, true),
        makeClip("skid",     prefix + "small_skid",    SMALL_RECT, 1.0f, false),
        makeClip("hurt",     prefix + "small_hurt",    SMALL_RECT, 1.0f, false),
        makeClip("dead",     prefix + "small_dead",    SMALL_RECT, 1.0f, false),
        makeClip("grow",     prefix + "small_grow",    SMALL_RECT, 8.0f, false),
        makeClip("flagpole", prefix + "small_climb",   SMALL_RECT, 4.0f, true),
        makeClip("pipe",     prefix + "small_idle",    SMALL_RECT, 1.0f, false),
        // Big Mario clips
        makeClip("big_idle",     prefix + "big_idle",    BIG_RECT, 1.0f, true),
        makeClip("big_run",      prefix + "big_run",     BIG_RECT, 10.0f, true),
        makeClip("big_jump",     prefix + "big_jump",    BIG_RECT, 1.0f, false),
        makeClip("big_fall",     prefix + "big_jump",    BIG_RECT, 1.0f, true),
        makeClip("big_skid",     prefix + "big_skid",    BIG_RECT, 1.0f, false),
        makeClip("big_flagpole", prefix + "big_climb",   BIG_RECT, 4.0f, true),
        // Fire Mario clips
        makeClip("fire_idle",     prefix + "fire_idle",    BIG_RECT, 1.0f, true),
        makeClip("fire_run",      prefix + "fire_run",     BIG_RECT, 10.0f, true),
        makeClip("fire_jump",     prefix + "fire_jump",    BIG_RECT, 1.0f, false),
        makeClip("fire_fall",     prefix + "fire_jump",    BIG_RECT, 1.0f, true),
        makeClip("fire_skid",     prefix + "fire_skid",    BIG_RECT, 1.0f, false),
        makeClip("fire_flagpole", prefix + "fire_climb",   BIG_RECT, 4.0f, true),
    };
    anim.currentClip = 0;
    registry.emplace<AnimationComponent>(entity, anim);

    PlayerComponent pc{};
    pc.playerIndex = playerIndex;
    registry.emplace<PlayerComponent>(entity, pc);

    registry.emplace<HealthComponent>(entity, HealthComponent{1, 1, 0, false});
    registry.emplace<TagComponent>(entity, TagComponent{playerIndex == 0 ? "mario" : "luigi"});

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

    float collW = 14.0f, collH = 14.0f;
    if (type == EnemyType::Bowser) {
        collW = 28.0f; collH = 30.0f;
    }
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {collW, collH}, false, false
    });

    std::string texKey;
    Rect rect = ENEMY_RECT;
    switch (type) {
        case EnemyType::Goomba:       texKey = "goomba";        break;
        case EnemyType::Koopa:        texKey = "koopa";         break;
        case EnemyType::PiranhaPlant: texKey = "piranha_plant"; break;
        case EnemyType::Bowser:       texKey = "bowser"; rect = BOWSER_RECT; break;
    }

    SpriteComponent sprite;
    sprite.texture = tex(texKey);
    sprite.srcRect = rect;
    sprite.zOrder  = 5;
    registry.emplace<SpriteComponent>(entity, sprite);

    AnimationComponent anim;
    anim.clips = {
        makeClip("idle",   texKey, rect, 1.0f, true),
        makeClip("walk",   texKey, rect, 6.0f, true),
        makeClip("attack", texKey, rect, 1.0f, false),
        makeClip("shell",  texKey, rect, 1.0f, true),
        makeClip("hurt",   texKey, rect, 1.0f, false),
        makeClip("dead",   texKey, rect, 1.0f, false),
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
    if (type == EnemyType::Bowser) {
        ec.hitsToKill = 5;
    }
    if (type == EnemyType::PiranhaPlant) {
        ec.bobBaseY = pos.y;
    }
    registry.emplace<EnemyComponent>(entity, ec);
}

// =============================================================================
// Goomba
// =============================================================================

entt::entity EntityFactory::createGoomba(entt::registry& registry, Vec2f pos,
                                          float patrolLeft, float patrolRight) {
    auto entity = registry.create();
    attachEnemyBase(registry, entity, pos, EnemyType::Goomba, 1,
                    patrolLeft, patrolRight, 1);
    registry.emplace<TagComponent>(entity, TagComponent{"goomba"});
    return entity;
}

// =============================================================================
// Koopa
// =============================================================================

entt::entity EntityFactory::createKoopa(entt::registry& registry, Vec2f pos,
                                         float patrolLeft, float patrolRight) {
    auto entity = registry.create();
    attachEnemyBase(registry, entity, pos, EnemyType::Koopa, 1,
                    patrolLeft, patrolRight, 1);
    registry.emplace<TagComponent>(entity, TagComponent{"koopa"});
    return entity;
}

// =============================================================================
// Piranha Plant
// =============================================================================

entt::entity EntityFactory::createPiranhaPlant(entt::registry& registry, Vec2f pos) {
    auto entity = registry.create();
    attachEnemyBase(registry, entity, pos, EnemyType::PiranhaPlant, 1,
                    pos.x, pos.x, 1);
    // Piranha plants don't fall — disable gravity
    registry.get<GravityComponent>(entity).multiplier = 0.0f;
    // Static collider — can't be pushed
    auto& coll = registry.get<ColliderComponent>(entity);
    coll.size = {14.0f, 24.0f};
    registry.emplace<TagComponent>(entity, TagComponent{"piranha_plant"});
    return entity;
}

// =============================================================================
// Bowser
// =============================================================================

entt::entity EntityFactory::createBowser(entt::registry& registry, Vec2f pos,
                                          float patrolLeft, float patrolRight) {
    auto entity = registry.create();
    attachEnemyBase(registry, entity, pos, EnemyType::Bowser, 5,
                    patrolLeft, patrolRight, -1);
    registry.emplace<TagComponent>(entity, TagComponent{"bowser"});
    return entity;
}

// =============================================================================
// Collectibles
// =============================================================================

entt::entity EntityFactory::createCoin(entt::registry& registry, Vec2f pos) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {14.0f, 14.0f}, true, false
    });

    SpriteComponent sprite;
    sprite.texture = tex("coin");
    sprite.srcRect = ITEM_RECT;
    sprite.zOrder  = 3;
    registry.emplace<SpriteComponent>(entity, sprite);

    AnimationComponent anim;
    anim.clips = {makeClip("idle", "coin", ITEM_RECT, 4.0f, true)};
    registry.emplace<AnimationComponent>(entity, anim);

    registry.emplace<CollectibleComponent>(entity, CollectibleComponent{
        CollectibleType::Coin, Config::COIN_VALUE, false, false
    });
    registry.emplace<TagComponent>(entity, TagComponent{"coin"});
    return entity;
}

entt::entity EntityFactory::createMushroom(entt::registry& registry, Vec2f pos, bool fromBlock) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<VelocityComponent>(entity, VelocityComponent{
        {80.0f, fromBlock ? -100.0f : 0.0f}
    });
    registry.emplace<GravityComponent>(entity);
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {14.0f, 14.0f}, true, false
    });

    SpriteComponent sprite;
    sprite.texture = tex("mushroom");
    sprite.srcRect = ITEM_RECT;
    sprite.zOrder  = 4;
    registry.emplace<SpriteComponent>(entity, sprite);

    registry.emplace<CollectibleComponent>(entity, CollectibleComponent{
        CollectibleType::Mushroom, 1000, false, fromBlock
    });
    registry.emplace<TagComponent>(entity, TagComponent{"mushroom"});
    return entity;
}

entt::entity EntityFactory::createFireFlower(entt::registry& registry, Vec2f pos, bool fromBlock) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {14.0f, 14.0f}, true, false
    });

    SpriteComponent sprite;
    sprite.texture = tex("fire_flower");
    sprite.srcRect = ITEM_RECT;
    sprite.zOrder  = 4;
    registry.emplace<SpriteComponent>(entity, sprite);

    AnimationComponent anim;
    anim.clips = {makeClip("idle", "fire_flower", ITEM_RECT, 4.0f, true)};
    registry.emplace<AnimationComponent>(entity, anim);

    registry.emplace<CollectibleComponent>(entity, CollectibleComponent{
        CollectibleType::FireFlower, 1000, false, fromBlock
    });
    registry.emplace<TagComponent>(entity, TagComponent{"fire_flower"});
    return entity;
}

entt::entity EntityFactory::createStar(entt::registry& registry, Vec2f pos, bool fromBlock) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<VelocityComponent>(entity, VelocityComponent{
        {120.0f, fromBlock ? -200.0f : 0.0f}
    });
    registry.emplace<GravityComponent>(entity);
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {14.0f, 14.0f}, true, false
    });

    SpriteComponent sprite;
    sprite.texture = tex("star");
    sprite.srcRect = ITEM_RECT;
    sprite.zOrder  = 4;
    registry.emplace<SpriteComponent>(entity, sprite);

    AnimationComponent anim;
    anim.clips = {makeClip("idle", "star", ITEM_RECT, 6.0f, true)};
    registry.emplace<AnimationComponent>(entity, anim);

    registry.emplace<CollectibleComponent>(entity, CollectibleComponent{
        CollectibleType::Star, 1000, false, fromBlock
    });
    registry.emplace<TagComponent>(entity, TagComponent{"star"});
    return entity;
}

entt::entity EntityFactory::createOneUp(entt::registry& registry, Vec2f pos, bool fromBlock) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<VelocityComponent>(entity, VelocityComponent{
        {60.0f, fromBlock ? -100.0f : 0.0f}
    });
    registry.emplace<GravityComponent>(entity);
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {14.0f, 14.0f}, true, false
    });

    SpriteComponent sprite;
    sprite.texture = tex("oneup_mushroom");
    sprite.srcRect = ITEM_RECT;
    sprite.zOrder  = 4;
    registry.emplace<SpriteComponent>(entity, sprite);

    registry.emplace<CollectibleComponent>(entity, CollectibleComponent{
        CollectibleType::OneUp, 0, false, fromBlock
    });
    registry.emplace<TagComponent>(entity, TagComponent{"1up"});
    return entity;
}

// =============================================================================
// World objects
// =============================================================================

entt::entity EntityFactory::createQuestionBlock(entt::registry& registry, Vec2f pos,
                                                 CollectibleType contents) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {32.0f, 32.0f}, false, true
    });

    SpriteComponent sprite;
    sprite.texture = tex("question_block");
    sprite.srcRect = TILE_RECT;
    sprite.zOrder  = 2;
    registry.emplace<SpriteComponent>(entity, sprite);

    AnimationComponent anim;
    anim.clips = {
        makeClip("active", "question_block", TILE_RECT, 4.0f, true),
        makeClip("empty",  "empty_block",    TILE_RECT, 1.0f, true),
    };
    registry.emplace<AnimationComponent>(entity, anim);

    registry.emplace<QuestionBlockComponent>(entity, QuestionBlockComponent{contents, false, 0.0f, 0.0f});
    registry.emplace<TagComponent>(entity, TagComponent{"question_block"});
    return entity;
}

entt::entity EntityFactory::createPipe(entt::registry& registry, Vec2f pos,
                                        bool enterable, Vec2f destination) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {64.0f, 64.0f}, false, true
    });

    SpriteComponent sprite;
    sprite.texture = tex("pipe");
    sprite.srcRect = PIPE_RECT;
    sprite.zOrder  = 2;
    registry.emplace<SpriteComponent>(entity, sprite);

    registry.emplace<PipeComponent>(entity, PipeComponent{enterable, destination, true});
    registry.emplace<TagComponent>(entity, TagComponent{"pipe"});
    return entity;
}

entt::entity EntityFactory::createFlagPole(entt::registry& registry, Vec2f pos, float height) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {14.0f, 0.0f}, {4.0f, height}, true, false
    });

    SpriteComponent sprite;
    sprite.texture = tex("flagpole");
    sprite.srcRect = {0.0f, 0.0f, 32.0f, height};
    sprite.zOrder  = 1;
    registry.emplace<SpriteComponent>(entity, sprite);

    registry.emplace<FlagPoleComponent>(entity, FlagPoleComponent{pos.y, pos.y + height, false});
    registry.emplace<GoalComponent>(entity);
    registry.emplace<TagComponent>(entity, TagComponent{"flagpole"});
    return entity;
}

entt::entity EntityFactory::createGoal(entt::registry& registry, Vec2f pos) {
    return createFlagPole(registry, pos, 288.0f); // 9 tiles tall
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
        {3.0f, 3.0f}, {10.0f, 10.0f}, true, false
    });

    SpriteComponent sprite;
    sprite.texture = tex("projectile");
    sprite.srcRect = {0.0f, 0.0f, 16.0f, 16.0f};
    sprite.zOrder  = 8;
    registry.emplace<SpriteComponent>(entity, sprite);

    registry.emplace<ProjectileComponent>(entity, ProjectileComponent{
        static_cast<uint32_t>(entt::to_integral(owner)),
        1, 3.0f, 300.0f, direction, false, false
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
        case ParticleEmitterComponent::Effect::CoinSparkle:
            emitter.lifetime      = 0.4f;
            emitter.particleCount = 5;
            emitter.color         = Color{255, 220, 50, 230};
            break;
        case ParticleEmitterComponent::Effect::BrickDebris:
            emitter.lifetime      = 0.6f;
            emitter.particleCount = 10;
            emitter.color         = Color{160, 100, 60, 220};
            break;
        case ParticleEmitterComponent::Effect::StompPoof:
            emitter.lifetime      = 0.3f;
            emitter.particleCount = 6;
            emitter.color         = Color{200, 200, 200, 200};
            break;
        case ParticleEmitterComponent::Effect::FireballBurst:
            emitter.lifetime      = 0.25f;
            emitter.particleCount = 4;
            emitter.color         = Color{255, 160, 40, 230};
            break;
        case ParticleEmitterComponent::Effect::PowerUpSparkle:
            emitter.lifetime      = 0.5f;
            emitter.particleCount = 8;
            emitter.color         = Color{255, 255, 200, 230};
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
