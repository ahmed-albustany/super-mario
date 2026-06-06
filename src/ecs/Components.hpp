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

/// @brief Mario power-up state — determines sprite set and abilities.
enum class MarioPowerState {
    Small,    ///< Default — one hit = death
    Big,      ///< After mushroom — can break bricks, one hit = shrink to Small
    Fire      ///< After fire flower — can shoot fireballs, one hit = shrink to Small
};

/// @brief All possible player states for the player state machine.
enum class PlayerState {
    Idle,
    Running,
    Jumping,
    Falling,
    Skidding,     ///< Turning around while running
    Growing,      ///< Power-up grow animation (brief invulnerable)
    Shrinking,    ///< Losing power (Big/Fire → Small)
    FlagPole,     ///< Sliding down flagpole
    EnteringPipe, ///< Entering a pipe
    Hurt,
    Dead
};

/// @brief Player-specific gameplay data.
struct PlayerComponent {
    PlayerState state       = PlayerState::Idle;
    MarioPowerState power   = MarioPowerState::Small;
    int   jumpCount         = 0;      ///< 0 = grounded, 1 = jumped
    bool  isGrounded        = false;
    int   facing            = 1;      ///< 1 = right, -1 = left
    float coyoteTimer       = 0.0f;   ///< Brief grace period after leaving ledge
    float jumpBufferTimer   = 0.0f;   ///< Pre-land jump buffer
    bool  isRunning         = false;  ///< Run button held
    float growTimer         = 0.0f;   ///< Timer for grow/shrink animation
    float pipeTimer         = 0.0f;   ///< Timer for pipe enter animation
    Vec2f pipeTarget;                 ///< Where pipe teleports player to
    int   playerIndex       = 0;      ///< 0 = Player 1 (Mario), 1 = Player 2 (Luigi)

    static constexpr float COYOTE_TIME    = 0.1f;   ///< ~6 frames at 60fps
    static constexpr float JUMP_BUFFER    = 0.1f;   ///< seconds
    static constexpr float GROW_DURATION  = 0.5f;   ///< seconds
    static constexpr float PIPE_DURATION  = 0.8f;   ///< seconds
};

// =============================================================================
// Enemies
// =============================================================================

/// @brief Enemy type variants — Mario style.
enum class EnemyType {
    Goomba,       ///< Walks, dies on stomp
    Koopa,        ///< Walks, becomes shell on stomp (can be kicked)
    PiranhaPlant, ///< Bobs up/down from pipe, can't be stomped
    Bowser        ///< Boss — shoots fire, takes multiple hits
};

/// @brief All possible enemy states.
enum class EnemyState {
    Idle,
    Patrol,
    Chase,
    Attack,
    Shell,      ///< Koopa shell state (stationary or sliding)
    Hurt,
    Dead
};

/// @brief Enemy-specific gameplay data.
struct EnemyComponent {
    EnemyType  type          = EnemyType::Goomba;
    EnemyState state         = EnemyState::Patrol;
    float patrolLeft         = 0.0f;   ///< Left bound of patrol area (world x)
    float patrolRight        = 0.0f;   ///< Right bound of patrol area (world x)
    int   facing             = 1;      ///< 1 = right, -1 = left
    bool  isAlerted          = false;  ///< Has spotted the player

    // Bowser-specific
    float shootCooldown      = 0.0f;   ///< Seconds until next shot
    float shootInterval      = 2.5f;   ///< Time between fire breaths
    int   hitsToKill         = 5;      ///< Bowser takes multiple hits

    // PiranhaPlant-specific
    float bobTimer           = 0.0f;
    float bobPhase           = 0.0f;   ///< 0..1 = position in bob cycle
    float bobBaseY           = 0.0f;   ///< Resting Y position (inside pipe)

    // Koopa-specific
    bool  isShell            = false;  ///< True when stomped into shell
    bool  shellMoving        = false;  ///< True when shell is sliding
    float shellSpeed         = 400.0f; ///< Shell slide speed

    // Death animation timer (e.g. Goomba flat for 0.5s before despawning)
    float deathTimer         = 0.0f;

    // Unused legacy fields kept for compatibility
    float bounceForce        = -500.0f;
    float bounceTimer        = 0.0f;
    bool  isArmored          = false;
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

/// @brief A projectile fired by an enemy (or player fireball).
struct ProjectileComponent {
    uint32_t ownerId     = 0;      ///< EnTT entity id of the shooter (to prevent self-hit)
    int   damage         = 1;
    float lifetime       = 3.0f;   ///< Seconds remaining before auto-destroy
    float speed          = 300.0f; ///< Pixels/second
    Vec2f direction      = {1.0f, 0.0f};
    bool  isFireball     = false;  ///< Player fireball (bounces, affected by gravity)
    bool  isBowserFire   = false;  ///< Bowser fire breath
};

// =============================================================================
// Collectibles & Power-ups
// =============================================================================

/// @brief Types of collectibles.
enum class CollectibleType {
    Coin,         ///< Standard coin — 100 coins = 1-up
    Mushroom,     ///< Makes Small Mario → Big Mario
    FireFlower,   ///< Makes Big Mario → Fire Mario (Small → Big)
    Star,         ///< Grants invincibility for a duration
    OneUp         ///< Extra life
};

/// @brief Marks an entity as a collectible that the player can pick up.
struct CollectibleComponent {
    CollectibleType type  = CollectibleType::Coin;
    int   value           = 200;   ///< Score value
    bool  collected       = false;  ///< Set to true on pickup, entity destroyed next frame
    bool  fromBlock       = false;  ///< Spawned from question block (has upward velocity)
};

/// @brief Active power-up effect on the player (Star invincibility).
enum class PowerUpType {
    StarInvincibility  ///< Cannot take damage, kills enemies on contact
};

/// @brief Tracks an active power-up buff with a countdown timer.
struct PowerUpComponent {
    PowerUpType type         = PowerUpType::StarInvincibility;
    float durationRemaining  = 0.0f;  ///< Seconds left
    float speedMultiplier    = 1.0f;
};

// =============================================================================
// World interaction
// =============================================================================

/// @brief Marks a tile as destructible (Big Mario can break bricks).
struct DestructibleComponent {
    int  hitsRemaining  = 1;    ///< Hits required to destroy
    bool destroyed      = false;
};

/// @brief Question block — hit from below to spawn an item.
struct QuestionBlockComponent {
    CollectibleType contents = CollectibleType::Coin; ///< What to spawn
    bool  isHit          = false;  ///< Already been hit
    float bumpTimer      = 0.0f;   ///< Bump animation timer
    float bumpOffset     = 0.0f;   ///< Visual offset during bump
};

/// @brief Pipe — player can enter by pressing down on top.
struct PipeComponent {
    bool  isEnterable    = false;  ///< Can the player enter this pipe?
    Vec2f destination;             ///< Where it teleports to (world coords)
    bool  isVertical     = true;   ///< true = enter from top, false = enter from side
};

/// @brief Flagpole end-of-level trigger.
struct FlagPoleComponent {
    float topY           = 0.0f;   ///< Y position of the top of the pole
    float bottomY        = 0.0f;   ///< Y position of the base
    bool  activated      = false;
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

/// @brief Floating score text that rises and fades out (e.g. "+100").
struct FloatingTextComponent {
    std::string text;
    float lifetime    = 0.8f;   ///< Total duration
    float elapsed     = 0.0f;
    float riseSpeed   = 80.0f;  ///< Pixels/second upward
    Color color       = Color{255, 255, 255, 255};
    int   fontSize    = 14;
};

/// @brief Particle effect burst.
struct ParticleEmitterComponent {
    enum class Effect {
        CoinSparkle,
        BrickDebris,
        StompPoof,
        FireballBurst,
        PowerUpSparkle,
        DeathPoof
    };

    Effect effect        = Effect::CoinSparkle;
    float  lifetime      = 0.5f;   ///< Total effect duration
    float  elapsed       = 0.0f;
    int    particleCount = 8;
    Color  color         = Color::White();
    bool   active        = false;
};
