#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "utils/Math.hpp"
#include "platform/IPlatform.hpp"

/// @file Components.hpp
/// @brief All ECS component structs — plain-old-data for cache-friendly iteration.
///        Used with EnTT registry. Each struct is a single component type.

// =============================================================================
// Spatial
// =============================================================================

struct TransformComponent {
    Vec2f position;
    Vec2f scale    = {1.0f, 1.0f};
    float rotation = 0.0f;
};

struct VelocityComponent {
    Vec2f velocity;
};

struct GravityComponent {
    float multiplier = 1.0f;
};

// =============================================================================
// Collision
// =============================================================================

struct ColliderComponent {
    Vec2f offset;
    Vec2f size;
    bool  isTrigger  = false;
    bool  isStatic   = false;

    [[nodiscard]] Rect toRect(const Vec2f& entityPos) const {
        return {entityPos.x + offset.x, entityPos.y + offset.y, size.x, size.y};
    }
};

// =============================================================================
// Rendering
// =============================================================================

struct SpriteComponent {
    TextureHandle texture;
    Rect  srcRect;
    int   zOrder     = 0;
    bool  flipX      = false;
    bool  visible    = true;
    Color tint       = Color::White();
};

/// @brief Frame-based sprite animation with sprite sheet support.
struct AnimationComponent {
    struct Clip {
        std::string name;
        std::vector<Rect> frames;
        float fps        = 10.0f;
        bool  loop       = true;
        TextureHandle texture;
    };

    std::vector<Clip> clips;
    int   currentClip   = 0;
    int   currentFrame  = 0;
    float frameTimer    = 0.0f;
    bool  finished      = false;

    // Sprite sheet extraction parameters
    int   frameWidth    = 32;   ///< Width of each frame in the sprite sheet
    int   frameHeight   = 32;   ///< Height of each frame in the sprite sheet
    int   frameCount    = 1;    ///< Total frames in the sprite sheet row
    float frameDuration = 0.1f; ///< Seconds per frame (alternative to fps)

    void play(const std::string& clipName) {
        for (int i = 0; i < static_cast<int>(clips.size()); ++i) {
            if (clips[static_cast<size_t>(i)].name == clipName) {
                if (currentClip != i) {
                    currentClip  = i;
                    currentFrame = 0;
                    frameTimer   = 0.0f;
                    finished     = false;
                }
                return;
            }
        }
    }

    [[nodiscard]] Rect currentSrcRect() const {
        if (clips.empty()) return {};
        const auto& clip = clips[static_cast<size_t>(currentClip)];
        if (clip.frames.empty()) return {};
        return clip.frames[static_cast<size_t>(currentFrame)];
    }
};

// =============================================================================
// Player
// =============================================================================

/// @brief All possible player states for the player state machine.
enum class PlayerState {
    Idle,
    Running,
    Jumping,
    DoubleJumping,
    Falling,
    WallJumping,
    Hit,
    Dead,
    Appearing,
    Disappearing
};

/// @brief Player-specific gameplay data.
struct PlayerComponent {
    PlayerState state       = PlayerState::Idle;
    int   jumpCount         = 0;      ///< 0 = grounded, 1 = jumped, 2 = double jumped
    bool  isGrounded        = false;
    int   facing            = 1;      ///< 1 = right, -1 = left
    float coyoteTimer       = 0.0f;
    float jumpBufferTimer   = 0.0f;
    int   playerIndex       = 0;      ///< 0-3

    // Death animation
    float deathTimer        = 0.0f;   ///< Countdown to respawn
    bool  deathAnimStarted  = false;

    static constexpr float COYOTE_TIME    = 0.1f;   ///< ~6 frames at 60fps
    static constexpr float JUMP_BUFFER    = 0.1f;

    // Legacy compatibility fields (unused but kept for compile)
    float growTimer         = 0.0f;
    float pipeTimer         = 0.0f;
    Vec2f pipeTarget;
    bool  isRunning         = false;
};

/// @brief Identifies which player index (0-3) an entity belongs to.
struct PlayerIndexComponent {
    int index = 0;
};

// =============================================================================
// Fruits (replaces collectibles)
// =============================================================================

/// @brief Fruit types from Pixel Adventure pack.
enum class FruitType {
    Cherry,
    Apple,
    Orange,
    Pineapple,
    Melon,
    Strawberry,
    Kiwi,
    Banana
};

/// @brief Marks an entity as a collectible fruit.
struct FruitComponent {
    FruitType type    = FruitType::Cherry;
    int   value       = 100;
    bool  collected   = false;
};

// =============================================================================
// Traps
// =============================================================================

/// @brief Trap type variants from Pixel Adventure.
enum class TrapType {
    Saw,
    SpikeHead,
    RockHead,
    Fire,
    Arrow,
    FallingPlatform,
    MovingPlatform,
    Fan,
    SpikedBall,
    Spikes,
    Trampoline,
    Blocks
};

/// @brief Trap-specific gameplay data.
struct TrapComponent {
    TrapType trapType       = TrapType::Spikes;
    bool  isActive          = true;
    float timer             = 0.0f;
    float speed             = 1.0f;

    // Fire-specific
    float onTime            = 2.0f;
    float offTime           = 1.0f;

    // Falling platform
    float shakeTimer        = 0.0f;
    bool  isFalling         = false;
    bool  playerOnTop       = false;

    // Arrow
    std::string direction   = "right";

    // Spiked ball
    float chainLength       = 8.0f;
    float swingAngle        = 0.0f;
    Vec2f anchorPos;

    // Fan
    float fanStrength       = 400.0f;

    // Moving platform path
    std::vector<Vec2f> path;
    int   pathIndex         = 0;
    bool  pathForward       = true;
};

// =============================================================================
// Checkpoints
// =============================================================================

/// @brief Checkpoint flag that saves player position on activation.
struct CheckpointComponent {
    bool  activated     = false;
    Vec2f respawnPos;
};

// =============================================================================
// Moving/Falling Platforms
// =============================================================================

/// @brief Platform behavior component.
enum class PlatformType {
    Moving,
    Falling
};

struct PlatformComponent {
    PlatformType type     = PlatformType::Moving;
    std::vector<Vec2f> path;
    int   pathIndex       = 0;
    float speed           = 60.0f;
    bool  forward         = true;

    // Falling-specific
    float shakeTimer      = 0.0f;
    bool  isFalling       = false;
    bool  playerOnTop     = false;
    Vec2f originalPos;
};

// =============================================================================
// Boxes (replaces question blocks)
// =============================================================================

/// @brief Box that spawns fruit when hit from below.
struct BoxComponent {
    int   hitsRemaining   = 3;
    bool  isHit           = false;
    bool  isBroken        = false;
    float bumpTimer       = 0.0f;
    float bumpOffset      = 0.0f;
    std::string boxType   = "box1"; ///< "box1", "box2", "box3"
};

// =============================================================================
// Combat (kept for compatibility)
// =============================================================================

struct HealthComponent {
    int hp                   = 1;
    int maxHP                = 1;
    int invincibilityFrames  = 0;
    bool isDead              = false;
};

struct ProjectileComponent {
    uint32_t ownerId     = 0;
    int   damage         = 1;
    float lifetime       = 3.0f;
    float speed          = 300.0f;
    Vec2f direction      = {1.0f, 0.0f};
    bool  isFireball     = false;
    bool  isBowserFire   = false;
};

// =============================================================================
// Enemies (kept for compatibility, not used in PIXEL RUSH)
// =============================================================================

enum class EnemyType {
    Goomba, Koopa, PiranhaPlant, Bowser
};

enum class EnemyState {
    Idle, Patrol, Chase, Attack, Shell, Hurt, Dead
};

struct EnemyComponent {
    EnemyType  type          = EnemyType::Goomba;
    EnemyState state         = EnemyState::Patrol;
    float patrolLeft         = 0.0f;
    float patrolRight        = 0.0f;
    int   facing             = 1;
    bool  isAlerted          = false;
    float shootCooldown      = 0.0f;
    float shootInterval      = 2.5f;
    int   hitsToKill         = 5;
    float bobTimer           = 0.0f;
    float bobPhase           = 0.0f;
    float bobBaseY           = 0.0f;
    bool  isShell            = false;
    bool  shellMoving        = false;
    float shellSpeed         = 400.0f;
    float deathTimer         = 0.0f;
    float bounceForce        = -500.0f;
    float bounceTimer        = 0.0f;
    bool  isArmored          = false;
};

// =============================================================================
// Legacy collectibles (kept for compatibility)
// =============================================================================

enum class CollectibleType {
    Coin, Mushroom, FireFlower, Star, OneUp
};

struct CollectibleComponent {
    CollectibleType type  = CollectibleType::Coin;
    int   value           = 200;
    bool  collected       = false;
    bool  fromBlock       = false;
};

enum class MarioPowerState {
    Small, Big, Fire
};

enum class PowerUpType {
    StarInvincibility
};

struct PowerUpComponent {
    PowerUpType type         = PowerUpType::StarInvincibility;
    float durationRemaining  = 0.0f;
    float speedMultiplier    = 1.0f;
};

struct DestructibleComponent {
    int  hitsRemaining  = 1;
    bool destroyed      = false;
};

struct QuestionBlockComponent {
    CollectibleType contents = CollectibleType::Coin;
    bool  isHit          = false;
    float bumpTimer      = 0.0f;
    float bumpOffset     = 0.0f;
};

struct PipeComponent {
    bool  isEnterable    = false;
    Vec2f destination;
    bool  isVertical     = true;
};

struct FlagPoleComponent {
    float topY           = 0.0f;
    float bottomY        = 0.0f;
    bool  activated      = false;
};

// =============================================================================
// Audio
// =============================================================================

struct AudioTriggerComponent {
    SoundHandle sound;
    bool triggerOnce    = true;
    bool hasTriggered   = false;
};

// =============================================================================
// Tags & metadata
// =============================================================================

struct TagComponent {
    std::string tag;
};

struct DestroyFlag {};

struct GoalComponent {
    bool reached = false;
};

struct FloatingTextComponent {
    std::string text;
    float lifetime    = 0.8f;
    float elapsed     = 0.0f;
    float riseSpeed   = 80.0f;
    Color color       = Color{255, 255, 255, 255};
    int   fontSize    = 14;
};

struct ParticleEmitterComponent {
    enum class Effect {
        CoinSparkle,
        BrickDebris,
        StompPoof,
        FireballBurst,
        PowerUpSparkle,
        DeathPoof,
        FruitCollect
    };

    Effect effect        = Effect::CoinSparkle;
    float  lifetime      = 0.5f;
    float  elapsed       = 0.0f;
    int    particleCount = 8;
    Color  color         = Color::White();
    bool   active        = false;
};
