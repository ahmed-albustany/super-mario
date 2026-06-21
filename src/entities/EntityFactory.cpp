#include "entities/EntityFactory.hpp"
#include "core/GameConfig.hpp"
#include "core/ResourceManager.hpp"

namespace {

TextureHandle tex(const std::string& key) {
    auto opt = ResourceManager::instance().getTexture(key);
    return opt.value_or(TextureHandle{0});
}

/// @brief Build sprite sheet animation clip: extracts frameCount frames of frameW x frameH
///        from a horizontal sprite sheet row.
AnimationComponent::Clip makeSheetClip(const std::string& name, const std::string& textureKey,
                                        int frameW, int frameH, int frameCount,
                                        float fps = 10.0f, bool loop = true) {
    AnimationComponent::Clip clip;
    clip.name    = name;
    clip.fps     = fps;
    clip.loop    = loop;
    clip.texture = tex(textureKey);

    for (int i = 0; i < frameCount; ++i) {
        clip.frames.push_back({
            static_cast<float>(i * frameW), 0.0f,
            static_cast<float>(frameW), static_cast<float>(frameH)
        });
    }
    return clip;
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

/// @brief Character texture key prefix by player index.
const char* characterPrefix(int playerIndex) {
    switch (playerIndex) {
        case 0:  return "char_mask_dude";
        case 1:  return "char_ninja_frog";
        case 2:  return "char_pink_man";
        case 3:  return "char_virtual_guy";
        default: return "char_mask_dude";
    }
}

const char* characterTag(int playerIndex) {
    switch (playerIndex) {
        case 0:  return "mask_dude";
        case 1:  return "ninja_frog";
        case 2:  return "pink_man";
        case 3:  return "virtual_guy";
        default: return "mask_dude";
    }
}

constexpr Rect CHAR_RECT  = {0.0f, 0.0f, 32.0f, 32.0f};
constexpr Rect FRUIT_RECT = {0.0f, 0.0f, 32.0f, 32.0f};
constexpr Rect TRAP_RECT  = {0.0f, 0.0f, 16.0f, 16.0f};
constexpr Rect ITEM_RECT  = {0.0f, 0.0f, 16.0f, 16.0f};
constexpr Rect TILE_RECT  = {0.0f, 0.0f, 32.0f, 32.0f};
constexpr Rect ENEMY_RECT = {0.0f, 0.0f, 16.0f, 16.0f};
constexpr Rect BOWSER_RECT = {0.0f, 0.0f, 32.0f, 32.0f};
constexpr Rect PIPE_RECT  = {0.0f, 0.0f, 64.0f, 64.0f};
constexpr Rect SMALL_RECT = {0.0f, 0.0f, 16.0f, 16.0f};
constexpr Rect BIG_RECT   = {0.0f, 0.0f, 16.0f, 32.0f};

} // anonymous namespace

// =============================================================================
// Player — Pixel Adventure characters
// =============================================================================

entt::entity EntityFactory::createPlayer(entt::registry& registry, Vec2f spawnPos,
                                          int playerIndex) {
    auto entity = registry.create();

    registry.emplace<TransformComponent>(entity, TransformComponent{spawnPos});
    registry.emplace<VelocityComponent>(entity);
    registry.emplace<GravityComponent>(entity);
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {4.0f, 4.0f}, {24.0f, 28.0f}, false, false
    });

    std::string prefix = characterPrefix(playerIndex);

    SpriteComponent sprite;
    sprite.texture = tex(prefix + "_idle");
    sprite.srcRect = CHAR_RECT;
    sprite.zOrder  = 10;
    registry.emplace<SpriteComponent>(entity, sprite);

    // Animation clips from sprite sheets
    // All characters: Idle=11f, Run=12f, Jump=1f, DoubleJump=6f, Fall=1f, Hit=7f, WallJump=5f
    AnimationComponent anim;
    anim.frameWidth  = 32;
    anim.frameHeight = 32;
    anim.clips = {
        makeSheetClip("idle",        prefix + "_idle",          32, 32, 11, 20.0f, true),
        makeSheetClip("run",         prefix + "_run",           32, 32, 12, 20.0f, true),
        makeSheetClip("jump",        prefix + "_jump",          32, 32, 1,  1.0f,  false),
        makeSheetClip("double_jump", prefix + "_double_jump",   32, 32, 6,  15.0f, true),
        makeSheetClip("fall",        prefix + "_fall",          32, 32, 1,  1.0f,  true),
        makeSheetClip("hit",         prefix + "_hit",           32, 32, 7,  14.0f, false),
        makeSheetClip("wall_jump",   prefix + "_wall_jump",     32, 32, 5,  10.0f, true),
        makeSheetClip("appearing",   "char_appearing",          96, 96, 7,  14.0f, false),
        makeSheetClip("disappearing","char_disappearing",       96, 96, 7,  14.0f, false),
    };
    anim.currentClip = 0;
    registry.emplace<AnimationComponent>(entity, anim);

    PlayerComponent pc{};
    pc.playerIndex = playerIndex;
    registry.emplace<PlayerComponent>(entity, pc);

    registry.emplace<PlayerIndexComponent>(entity, PlayerIndexComponent{playerIndex});
    registry.emplace<HealthComponent>(entity, HealthComponent{1, 1, 0, false});
    registry.emplace<TagComponent>(entity, TagComponent{characterTag(playerIndex)});

    return entity;
}

// =============================================================================
// Fruits
// =============================================================================

entt::entity EntityFactory::createFruit(entt::registry& registry, Vec2f pos,
                                         FruitType type, int value) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {4.0f, 4.0f}, {24.0f, 24.0f}, true, false
    });

    std::string texKey;
    switch (type) {
        case FruitType::Cherry:     texKey = "fruit_cherry"; break;
        case FruitType::Apple:      texKey = "fruit_apple"; break;
        case FruitType::Orange:     texKey = "fruit_orange"; break;
        case FruitType::Pineapple:  texKey = "fruit_pineapple"; break;
        case FruitType::Melon:      texKey = "fruit_melon"; break;
        case FruitType::Strawberry: texKey = "fruit_strawberry"; break;
        case FruitType::Kiwi:       texKey = "fruit_kiwi"; break;
        case FruitType::Banana:     texKey = "fruit_banana"; break;
    }

    SpriteComponent sprite;
    sprite.texture = tex(texKey);
    sprite.srcRect = FRUIT_RECT;
    sprite.zOrder  = 4;
    registry.emplace<SpriteComponent>(entity, sprite);

    // Fruit animation: 17 frames, 32x32 each
    AnimationComponent anim;
    anim.clips = {makeSheetClip("idle", texKey, 32, 32, 17, 20.0f, true)};
    registry.emplace<AnimationComponent>(entity, anim);

    registry.emplace<FruitComponent>(entity, FruitComponent{type, value, false});
    registry.emplace<TagComponent>(entity, TagComponent{"fruit"});
    return entity;
}

entt::entity EntityFactory::createFruitByName(entt::registry& registry, Vec2f pos,
                                               const std::string& typeName) {
    FruitType type = FruitType::Cherry;
    int value = Config::FRUIT_CHERRY_VALUE;

    if (typeName == "apple")           { type = FruitType::Apple;      value = Config::FRUIT_APPLE_VALUE; }
    else if (typeName == "orange")     { type = FruitType::Orange;     value = Config::FRUIT_ORANGE_VALUE; }
    else if (typeName == "pineapple")  { type = FruitType::Pineapple;  value = Config::FRUIT_PINEAPPLE_VALUE; }
    else if (typeName == "melon")      { type = FruitType::Melon;      value = Config::FRUIT_MELON_VALUE; }
    else if (typeName == "strawberry") { type = FruitType::Strawberry; value = Config::FRUIT_STRAWBERRY_VALUE; }
    else if (typeName == "kiwi")       { type = FruitType::Kiwi;       value = Config::FRUIT_KIWI_VALUE; }
    else if (typeName == "banana")     { type = FruitType::Banana;     value = Config::FRUIT_BANANA_VALUE; }

    return createFruit(registry, pos, type, value);
}

// =============================================================================
// Traps
// =============================================================================

entt::entity EntityFactory::createTrap(entt::registry& registry, Vec2f pos,
                                        TrapType type, float speed) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});

    bool isTrigger = true;
    bool isStatic  = false;
    Vec2f collSize = {16.0f, 16.0f};
    std::string texKey = "trap_spikes";

    switch (type) {
        case TrapType::Saw:
            texKey = "trap_saw_on";
            collSize = {38.0f, 38.0f};
            break;
        case TrapType::Spikes:
            texKey = "trap_spikes";
            collSize = {16.0f, 16.0f};
            isStatic = true;
            break;
        case TrapType::SpikeHead:
            texKey = "trap_spike_head_idle";
            collSize = {44.0f, 44.0f};
            break;
        case TrapType::RockHead:
            texKey = "trap_rock_head_idle";
            collSize = {42.0f, 42.0f};
            registry.emplace<VelocityComponent>(entity);
            break;
        case TrapType::Trampoline:
            texKey = "trap_trampoline_idle";
            collSize = {28.0f, 28.0f};
            break;
        default:
            break;
    }

    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, collSize, isTrigger, isStatic
    });

    SpriteComponent sprite;
    sprite.texture = tex(texKey);
    sprite.srcRect = {0.0f, 0.0f, collSize.x, collSize.y};
    sprite.zOrder  = 5;
    registry.emplace<SpriteComponent>(entity, sprite);

    TrapComponent trap;
    trap.trapType = type;
    trap.speed = speed;
    registry.emplace<TrapComponent>(entity, trap);

    registry.emplace<TagComponent>(entity, TagComponent{"trap"});
    return entity;
}

entt::entity EntityFactory::createMovingPlatform(entt::registry& registry, Vec2f pos,
                                                  const std::vector<Vec2f>& path, float speed) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {32.0f, 8.0f}, false, true
    });

    SpriteComponent sprite;
    sprite.texture = tex("trap_platform");
    sprite.srcRect = {0.0f, 0.0f, 32.0f, 8.0f};
    sprite.zOrder  = 3;
    registry.emplace<SpriteComponent>(entity, sprite);

    TrapComponent trap;
    trap.trapType = TrapType::MovingPlatform;
    trap.path = path;
    trap.speed = speed;
    registry.emplace<TrapComponent>(entity, trap);

    registry.emplace<TagComponent>(entity, TagComponent{"moving_platform"});
    return entity;
}

entt::entity EntityFactory::createFallingPlatform(entt::registry& registry, Vec2f pos) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<VelocityComponent>(entity);
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {32.0f, 8.0f}, false, true
    });

    SpriteComponent sprite;
    sprite.texture = tex("trap_falling_platform_on");
    sprite.srcRect = {0.0f, 0.0f, 32.0f, 10.0f};
    sprite.zOrder  = 3;
    registry.emplace<SpriteComponent>(entity, sprite);

    TrapComponent trap;
    trap.trapType = TrapType::FallingPlatform;
    registry.emplace<TrapComponent>(entity, trap);

    registry.emplace<TagComponent>(entity, TagComponent{"falling_platform"});
    return entity;
}

entt::entity EntityFactory::createSpikedBall(entt::registry& registry, Vec2f anchorPos,
                                              float chainLength) {
    auto entity = registry.create();
    Vec2f ballPos = {anchorPos.x, anchorPos.y + chainLength * 16.0f};
    registry.emplace<TransformComponent>(entity, TransformComponent{ballPos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {28.0f, 28.0f}, true, false
    });

    SpriteComponent sprite;
    sprite.texture = tex("trap_spiked_ball");
    sprite.srcRect = {0.0f, 0.0f, 28.0f, 28.0f};
    sprite.zOrder  = 5;
    registry.emplace<SpriteComponent>(entity, sprite);

    TrapComponent trap;
    trap.trapType = TrapType::SpikedBall;
    trap.chainLength = chainLength;
    trap.anchorPos = anchorPos;
    trap.speed = 1.5f;
    registry.emplace<TrapComponent>(entity, trap);

    registry.emplace<TagComponent>(entity, TagComponent{"spiked_ball"});
    return entity;
}

entt::entity EntityFactory::createFan(entt::registry& registry, Vec2f pos, float strength) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    // Fan trigger area extends upward
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, -128.0f}, {24.0f, 128.0f}, true, false
    });

    SpriteComponent sprite;
    sprite.texture = tex("trap_fan_on");
    sprite.srcRect = {0.0f, 0.0f, 24.0f, 8.0f};
    sprite.zOrder  = 3;
    registry.emplace<SpriteComponent>(entity, sprite);

    TrapComponent trap;
    trap.trapType = TrapType::Fan;
    trap.fanStrength = strength;
    registry.emplace<TrapComponent>(entity, trap);

    registry.emplace<TagComponent>(entity, TagComponent{"fan"});
    return entity;
}

entt::entity EntityFactory::createArrow(entt::registry& registry, Vec2f pos,
                                         const std::string& direction) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {16.0f, 16.0f}, false, true
    });

    SpriteComponent sprite;
    sprite.texture = tex("trap_arrow");
    sprite.srcRect = {0.0f, 0.0f, 16.0f, 16.0f};
    sprite.zOrder  = 5;
    sprite.flipX = (direction == "left");
    registry.emplace<SpriteComponent>(entity, sprite);

    TrapComponent trap;
    trap.trapType = TrapType::Arrow;
    trap.direction = direction;
    registry.emplace<TrapComponent>(entity, trap);

    registry.emplace<TagComponent>(entity, TagComponent{"arrow_trap"});
    return entity;
}

entt::entity EntityFactory::createFire(entt::registry& registry, Vec2f pos,
                                        float onTime, float offTime) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {16.0f, 32.0f}, true, false
    });

    SpriteComponent sprite;
    sprite.texture = tex("trap_fire_on");
    sprite.srcRect = {0.0f, 0.0f, 16.0f, 32.0f};
    sprite.zOrder  = 5;
    registry.emplace<SpriteComponent>(entity, sprite);

    TrapComponent trap;
    trap.trapType = TrapType::Fire;
    trap.onTime = onTime;
    trap.offTime = offTime;
    trap.isActive = true;
    registry.emplace<TrapComponent>(entity, trap);

    registry.emplace<TagComponent>(entity, TagComponent{"fire_trap"});
    return entity;
}

// =============================================================================
// Boxes
// =============================================================================

entt::entity EntityFactory::createBox(entt::registry& registry, Vec2f pos,
                                       const std::string& boxType, int hits) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {28.0f, 24.0f}, false, true
    });

    // boxType is "box1", "box2", "box3" — manifest keys are box_idle1, box_hit, box_break
    std::string boxNum = boxType.substr(3); // "1", "2", "3"
    std::string idleKey = "box_idle" + boxNum;
    SpriteComponent sprite;
    sprite.texture = tex(idleKey);
    sprite.srcRect = {0.0f, 0.0f, 28.0f, 24.0f};
    sprite.zOrder  = 3;
    registry.emplace<SpriteComponent>(entity, sprite);

    AnimationComponent anim;
    anim.clips = {
        makeClip("idle",  idleKey,        {0.0f, 0.0f, 28.0f, 24.0f}, 1.0f, true),
        makeSheetClip("hit",   "box_hit",   28, 24, 3, 10.0f, false),
        makeSheetClip("break", "box_break", 28, 24, 4, 10.0f, false),
    };
    registry.emplace<AnimationComponent>(entity, anim);

    BoxComponent box;
    box.hitsRemaining = hits;
    box.boxType = boxType;
    registry.emplace<BoxComponent>(entity, box);

    registry.emplace<TagComponent>(entity, TagComponent{"box"});
    return entity;
}

// =============================================================================
// Checkpoints & Goals
// =============================================================================

entt::entity EntityFactory::createCheckpoint(entt::registry& registry, Vec2f pos) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {32.0f, 64.0f}, true, false
    });

    SpriteComponent sprite;
    sprite.texture = tex("checkpoint_no_flag");
    sprite.srcRect = {0.0f, 0.0f, 64.0f, 64.0f};
    sprite.zOrder  = 2;
    registry.emplace<SpriteComponent>(entity, sprite);

    AnimationComponent anim;
    anim.clips = {
        makeClip("no_flag",  "checkpoint_no_flag",   {0.0f, 0.0f, 64.0f, 64.0f}, 1.0f, true),
        makeSheetClip("flag_out",  "checkpoint_flag_out",  64, 64, 26, 20.0f, false),
        makeSheetClip("flag_idle", "checkpoint_flag_idle", 64, 64, 10, 20.0f, true),
    };
    registry.emplace<AnimationComponent>(entity, anim);

    CheckpointComponent cp;
    cp.respawnPos = pos;
    registry.emplace<CheckpointComponent>(entity, cp);

    registry.emplace<TagComponent>(entity, TagComponent{"checkpoint"});
    return entity;
}

entt::entity EntityFactory::createTrophy(entt::registry& registry, Vec2f pos) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{
        {0.0f, 0.0f}, {64.0f, 64.0f}, true, false
    });

    SpriteComponent sprite;
    sprite.texture = tex("trophy_idle");
    sprite.srcRect = {0.0f, 0.0f, 64.0f, 64.0f};
    sprite.zOrder  = 2;
    registry.emplace<SpriteComponent>(entity, sprite);

    AnimationComponent anim;
    anim.clips = {
        makeSheetClip("idle", "trophy_idle", 64, 64, 4, 8.0f, true),
        makeSheetClip("pressed", "trophy_pressed", 64, 64, 8, 15.0f, false),
    };
    registry.emplace<AnimationComponent>(entity, anim);

    registry.emplace<GoalComponent>(entity);
    registry.emplace<TagComponent>(entity, TagComponent{"trophy"});
    return entity;
}

entt::entity EntityFactory::createStartSign(entt::registry& registry, Vec2f pos) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});

    SpriteComponent sprite;
    sprite.texture = tex("start_idle");
    sprite.srcRect = {0.0f, 0.0f, 64.0f, 64.0f};
    sprite.zOrder  = 1;
    registry.emplace<SpriteComponent>(entity, sprite);

    registry.emplace<TagComponent>(entity, TagComponent{"start_sign"});
    return entity;
}

// =============================================================================
// Effects
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
        case ParticleEmitterComponent::Effect::FruitCollect:
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

entt::entity EntityFactory::createFloatingText(entt::registry& registry, Vec2f pos,
                                                const std::string& text, Color color) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});

    FloatingTextComponent ft{};
    ft.text     = text;
    ft.color    = color;
    ft.lifetime = 0.8f;
    ft.elapsed  = 0.0f;
    registry.emplace<FloatingTextComponent>(entity, ft);
    registry.emplace<TagComponent>(entity, TagComponent{"floating_text"});

    return entity;
}

// =============================================================================
// Legacy entity factories (kept for backward compatibility)
// =============================================================================

void EntityFactory::attachEnemyBase(entt::registry& registry, entt::entity entity,
                                    Vec2f pos, EnemyType type, int hp,
                                    float patrolLeft, float patrolRight,
                                    int facing) {
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<VelocityComponent>(entity);
    registry.emplace<GravityComponent>(entity);

    float collW = 14.0f, collH = 14.0f;
    if (type == EnemyType::Bowser) { collW = 28.0f; collH = 30.0f; }
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
    registry.emplace<AnimationComponent>(entity, anim);

    registry.emplace<HealthComponent>(entity, HealthComponent{hp, hp, 0, false});

    EnemyComponent ec{};
    ec.type        = type;
    ec.state       = EnemyState::Patrol;
    ec.patrolLeft  = patrolLeft;
    ec.patrolRight = patrolRight;
    ec.facing      = facing;
    if (type == EnemyType::Bowser) ec.hitsToKill = 5;
    if (type == EnemyType::PiranhaPlant) ec.bobBaseY = pos.y;
    registry.emplace<EnemyComponent>(entity, ec);
}

entt::entity EntityFactory::createGoomba(entt::registry& registry, Vec2f pos,
                                          float patrolLeft, float patrolRight) {
    auto entity = registry.create();
    attachEnemyBase(registry, entity, pos, EnemyType::Goomba, 1, patrolLeft, patrolRight, 1);
    registry.emplace<TagComponent>(entity, TagComponent{"goomba"});
    return entity;
}

entt::entity EntityFactory::createKoopa(entt::registry& registry, Vec2f pos,
                                         float patrolLeft, float patrolRight) {
    auto entity = registry.create();
    attachEnemyBase(registry, entity, pos, EnemyType::Koopa, 1, patrolLeft, patrolRight, 1);
    registry.emplace<TagComponent>(entity, TagComponent{"koopa"});
    return entity;
}

entt::entity EntityFactory::createPiranhaPlant(entt::registry& registry, Vec2f pos) {
    auto entity = registry.create();
    attachEnemyBase(registry, entity, pos, EnemyType::PiranhaPlant, 1, pos.x, pos.x, 1);
    registry.get<GravityComponent>(entity).multiplier = 0.0f;
    registry.get<ColliderComponent>(entity).size = {14.0f, 24.0f};
    registry.emplace<TagComponent>(entity, TagComponent{"piranha_plant"});
    return entity;
}

entt::entity EntityFactory::createBowser(entt::registry& registry, Vec2f pos,
                                          float patrolLeft, float patrolRight) {
    auto entity = registry.create();
    attachEnemyBase(registry, entity, pos, EnemyType::Bowser, 5, patrolLeft, patrolRight, -1);
    registry.emplace<TagComponent>(entity, TagComponent{"bowser"});
    return entity;
}

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
    registry.emplace<VelocityComponent>(entity, VelocityComponent{{80.0f, fromBlock ? -100.0f : 0.0f}});
    registry.emplace<GravityComponent>(entity);
    registry.emplace<ColliderComponent>(entity, ColliderComponent{{0.0f, 0.0f}, {14.0f, 14.0f}, true, false});
    SpriteComponent sprite;
    sprite.texture = tex("mushroom");
    sprite.srcRect = ITEM_RECT;
    sprite.zOrder  = 4;
    registry.emplace<SpriteComponent>(entity, sprite);
    registry.emplace<CollectibleComponent>(entity, CollectibleComponent{CollectibleType::Mushroom, 1000, false, fromBlock});
    registry.emplace<TagComponent>(entity, TagComponent{"mushroom"});
    return entity;
}

entt::entity EntityFactory::createFireFlower(entt::registry& registry, Vec2f pos, bool fromBlock) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{{0.0f, 0.0f}, {14.0f, 14.0f}, true, false});
    SpriteComponent sprite;
    sprite.texture = tex("fire_flower");
    sprite.srcRect = ITEM_RECT;
    sprite.zOrder  = 4;
    registry.emplace<SpriteComponent>(entity, sprite);
    registry.emplace<CollectibleComponent>(entity, CollectibleComponent{CollectibleType::FireFlower, 1000, false, fromBlock});
    registry.emplace<TagComponent>(entity, TagComponent{"fire_flower"});
    return entity;
}

entt::entity EntityFactory::createStar(entt::registry& registry, Vec2f pos, bool fromBlock) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<VelocityComponent>(entity, VelocityComponent{{120.0f, fromBlock ? -200.0f : 0.0f}});
    registry.emplace<GravityComponent>(entity);
    registry.emplace<ColliderComponent>(entity, ColliderComponent{{0.0f, 0.0f}, {14.0f, 14.0f}, true, false});
    SpriteComponent sprite;
    sprite.texture = tex("star");
    sprite.srcRect = ITEM_RECT;
    sprite.zOrder  = 4;
    registry.emplace<SpriteComponent>(entity, sprite);
    registry.emplace<CollectibleComponent>(entity, CollectibleComponent{CollectibleType::Star, 1000, false, fromBlock});
    registry.emplace<TagComponent>(entity, TagComponent{"star"});
    return entity;
}

entt::entity EntityFactory::createOneUp(entt::registry& registry, Vec2f pos, bool fromBlock) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<VelocityComponent>(entity, VelocityComponent{{60.0f, fromBlock ? -100.0f : 0.0f}});
    registry.emplace<GravityComponent>(entity);
    registry.emplace<ColliderComponent>(entity, ColliderComponent{{0.0f, 0.0f}, {14.0f, 14.0f}, true, false});
    SpriteComponent sprite;
    sprite.texture = tex("one_up");
    sprite.srcRect = ITEM_RECT;
    sprite.zOrder  = 4;
    registry.emplace<SpriteComponent>(entity, sprite);
    registry.emplace<CollectibleComponent>(entity, CollectibleComponent{CollectibleType::OneUp, 0, false, fromBlock});
    registry.emplace<TagComponent>(entity, TagComponent{"1up"});
    return entity;
}

entt::entity EntityFactory::createQuestionBlock(entt::registry& registry, Vec2f pos,
                                                 CollectibleType contents) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{{0.0f, 0.0f}, {32.0f, 32.0f}, false, true});
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
    registry.emplace<ColliderComponent>(entity, ColliderComponent{{0.0f, 0.0f}, {64.0f, 64.0f}, false, true});
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
    registry.emplace<ColliderComponent>(entity, ColliderComponent{{14.0f, 0.0f}, {4.0f, height}, true, false});
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
    return createTrophy(registry, pos);
}

entt::entity EntityFactory::createProjectile(entt::registry& registry, Vec2f pos,
                                              Vec2f direction, entt::entity owner) {
    auto entity = registry.create();
    registry.emplace<TransformComponent>(entity, TransformComponent{pos});
    registry.emplace<VelocityComponent>(entity, VelocityComponent{{direction.x * 300.0f, direction.y * 300.0f}});
    registry.emplace<ColliderComponent>(entity, ColliderComponent{{3.0f, 3.0f}, {10.0f, 10.0f}, true, false});
    SpriteComponent sprite;
    sprite.srcRect = {0.0f, 0.0f, 16.0f, 16.0f};
    sprite.zOrder  = 8;
    registry.emplace<SpriteComponent>(entity, sprite);
    registry.emplace<ProjectileComponent>(entity, ProjectileComponent{
        static_cast<uint32_t>(entt::to_integral(owner)), 1, 3.0f, 300.0f, direction, false, false
    });
    registry.emplace<TagComponent>(entity, TagComponent{"projectile"});
    return entity;
}
