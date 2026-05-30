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

/// @brief Position, scale, and rotation in world space.
struct TransformComponent {
    Vec2f position;
    Vec2f scale    = {1.0f, 1.0f};
    float rotation = 0.0f;
};

/// @brief Current velocity in pixels/second.
struct VelocityComponent {
    Vec2f velocity;
};

/// @brief Marks an entity as affected by gravity.
struct GravityComponent {
    float multiplier = 1.0f;  ///< Scale factor (e.g. 0.0 = no gravity, 0.5 = half)
};

// =============================================================================
// Collision
// =============================================================================

/// @brief Axis-aligned bounding box collider, relative to TransformComponent position.
struct ColliderComponent {
    Vec2f offset;       ///< Offset from entity position to collider top-left
    Vec2f size;         ///< Width and height of the collider
    bool  isTrigger  = false;  ///< Triggers fire events but don't block movement
    bool  isStatic   = false;  ///< Static colliders never move (tiles, walls)

    /// @brief Build the world-space AABB given the entity's position.
    [[nodiscard]] Rect toRect(const Vec2f& entityPos) const {
        return {entityPos.x + offset.x, entityPos.y + offset.y, size.x, size.y};
    }
};

// =============================================================================
// Rendering
// =============================================================================

/// @brief Sprite rendering data — references a loaded texture via handle.
struct SpriteComponent {
    TextureHandle texture;
    Rect  srcRect;           ///< Current frame rectangle in the spritesheet
    int   zOrder     = 0;    ///< Higher z-order renders on top
    bool  flipX      = false;
    bool  visible    = true;
    Color tint       = Color::White();
};

/// @brief Frame-based sprite animation.
struct AnimationComponent {
    /// @brief A single animation clip (e.g. "run", "idle", "jump").
    struct Clip {
        std::string name;
        std::vector<Rect> frames;   ///< Source rects for each frame
        float fps        = 10.0f;
        bool  loop       = true;
        TextureHandle texture;       ///< If valid, sets sprite texture when this clip plays
    };

    std::vector<Clip> clips;         ///< All animation clips for this entity
    int   currentClip   = 0;         ///< Index into clips[]
    int   currentFrame  = 0;         ///< Current frame within the active clip
    float frameTimer    = 0.0f;      ///< Accumulator for frame advance
    bool  finished      = false;     ///< True if non-looping clip has ended

    /// @brief Set the active clip by name. Resets frame to 0.
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

    /// @brief Get the source rect for the current frame of the active clip.
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
    Dashing,
    WallSliding,
    WallJumping,
    Hurt,
    Dead
};

/// @brief Player-specific gameplay data.
struct PlayerComponent {
    PlayerState state       = PlayerState::Idle;
    int   jumpCount         = 0;      ///< 0 = grounded, 1 = single jumped, 2 = double jumped
    float dashCooldownTimer = 0.0f;   ///< Seconds remaining before dash is available
    float dashTimer         = 0.0f;   ///< Seconds remaining in current dash
    bool  isGrounded        = false;
    bool  isTouchingWallLeft  = false;
    bool  isTouchingWallRight = false;
    int   facing            = 1;      ///< 1 = right, -1 = left
    float coyoteTimer       = 0.0f;   ///< Brief grace period after leaving ledge
    float jumpBufferTimer   = 0.0f;   ///< Pre-land jump buffer

    static constexpr float COYOTE_TIME    = 0.08f;  ///< seconds
    static constexpr float JUMP_BUFFER    = 0.1f;   ///< seconds
};

// =============================================================================
// Enemies
// =============================================================================

/// @brief Enemy type variants.
enum class EnemyType {
    Walker,     ///< Patrols edge-to-edge on a platform
    Jumper,     ///< Bounces, gravity-affected
    Shooter,    ///< Fires projectiles at player on sight
    Guardian    ///< Armored, requires 2 hits, flashes on first hit
};

/// @brief All possible enemy states.
enum class EnemyState {
    Idle,
    Patrol,
    Chase,
    Attack,
    Hurt,
    Dead
};

/// @brief Enemy-specific gameplay data.
struct EnemyComponent {
    EnemyType  type          = EnemyType::Walker;
    EnemyState state         = EnemyState::Patrol;
    float patrolLeft         = 0.0f;   ///< Left bound of patrol area (world x)
    float patrolRight        = 0.0f;   ///< Right bound of patrol area (world x)
    int   facing             = 1;      ///< 1 = right, -1 = left
    bool  isAlerted          = false;  ///< Has spotted the player

    // Shooter-specific
    float shootCooldown      = 0.0f;   ///< Seconds until next shot
    float shootInterval      = 2.0f;   ///< Time between shots

    // Jumper-specific
    float bounceForce        = -500.0f; ///< Vertical velocity on bounce
    float bounceTimer        = 0.0f;

    // Guardian-specific
    bool  isArmored          = true;   ///< If true, takes reduced damage / flashes
};

// =============================================================================
// Combat
// =============================================================================

/// @brief Health and invincibility tracking.
struct HealthComponent {
    int hp                   = 1;
    int maxHP                = 1;
    int invincibilityFrames  = 0;     ///< Remaining i-frames (counts down each tick)
    bool isDead              = false;
};

/// @brief A projectile fired by an enemy (or player power-up).
struct ProjectileComponent {
    uint32_t ownerId     = 0;      ///< EnTT entity id of the shooter (to prevent self-hit)
    int   damage         = 1;
    float lifetime       = 3.0f;   ///< Seconds remaining before auto-destroy
    float speed          = 300.0f; ///< Pixels/second
    Vec2f direction      = {1.0f, 0.0f};
};

// =============================================================================
// Collectibles & Power-ups
// =============================================================================

/// @brief Types of collectibles.
enum class CollectibleType {
    Coin,         ///< Standard score pickup
    GemShard,     ///< Rare, unlocks secret level
    PowerCrystal  ///< Grants invincibility + speed boost
};

/// @brief Marks an entity as a collectible that the player can pick up.
struct CollectibleComponent {
    CollectibleType type  = CollectibleType::Coin;
    int   value           = 100;   ///< Score value or special meaning
    bool  collected       = false;  ///< Set to true on pickup, entity destroyed next frame
};

/// @brief Active power-up effect on the player.
enum class PowerUpType {
    Invincibility,  ///< Cannot take damage
    SpeedBoost      ///< Increased movement speed
};

/// @brief Tracks an active power-up buff with a countdown timer.
struct PowerUpComponent {
    PowerUpType type         = PowerUpType::Invincibility;
    float durationRemaining  = 0.0f;  ///< Seconds left
    float speedMultiplier    = 1.5f;  ///< Only used for SpeedBoost
};

// =============================================================================
// World interaction
// =============================================================================

/// @brief Marks a tile as destructible (can be dashed through).
struct DestructibleComponent {
    int  hitsRemaining  = 1;    ///< Hits required to destroy
    bool destroyed      = false;
};

// =============================================================================
// Audio
// =============================================================================

/// @brief Triggers a sound effect when a condition is met.
struct AudioTriggerComponent {
    SoundHandle sound;
    bool triggerOnce    = true;  ///< If true, fires only the first time
    bool hasTriggered   = false;
};

// =============================================================================
// Tags & metadata
// =============================================================================

/// @brief String tag for debugging / editor identification.
struct TagComponent {
    std::string tag;
};

/// @brief Marks an entity for deferred destruction at end of frame.
struct DestroyFlag {};

/// @brief Marks the level goal (flagpole / exit portal).
struct GoalComponent {
    bool reached = false;
};

/// @brief Particle effect burst (e.g. double-jump puff, dash afterimage).
struct ParticleEmitterComponent {
    enum class Effect {
        DoubleJumpBurst,
        DashAfterimage,
        CoinSparkle,
        DestructionDebris,
        DeathPoof
    };

    Effect effect        = Effect::CoinSparkle;
    float  lifetime      = 0.5f;   ///< Total effect duration
    float  elapsed       = 0.0f;
    int    particleCount = 8;
    Color  color         = Color::White();
    bool   active        = false;
};
