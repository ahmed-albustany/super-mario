#include "ecs/systems/PlayerSystem.hpp"
#include "ecs/Components.hpp"
#include "core/InputManager.hpp"
#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "core/GameConfig.hpp"
#include "core/ResourceManager.hpp"
#include "utils/Math.hpp"

#include <cmath>

namespace {

/// @brief Resolve input for a specific player index.
struct PlayerInput {
    float moveX = 0.0f;
    bool  moveDown = false;
    bool  jumpPressed = false;
    bool  jumpReleased = false;
    bool  runHeld = false;
    bool  runPressed = false;

    static PlayerInput read(const InputManager& input, int playerIndex) {
        PlayerInput pi;
        if (playerIndex == 0) {
            if (input.isHeld(Action::MoveLeft))  pi.moveX -= 1.0f;
            if (input.isHeld(Action::MoveRight)) pi.moveX += 1.0f;
            pi.moveDown     = input.isHeld(Action::MoveDown);
            pi.jumpPressed  = input.isJustPressed(Action::Jump);
            pi.jumpReleased = input.isJustReleased(Action::Jump);
            pi.runHeld      = input.isHeld(Action::Run);
            pi.runPressed   = input.isJustPressed(Action::Run);
        } else {
            if (input.isHeldP2(Action::MoveLeft))  pi.moveX -= 1.0f;
            if (input.isHeldP2(Action::MoveRight)) pi.moveX += 1.0f;
            pi.moveDown     = input.isHeldP2(Action::MoveDown);
            pi.jumpPressed  = input.isJustPressedP2(Action::Jump);
            pi.jumpReleased = input.isJustReleasedP2(Action::Jump);
            pi.runHeld      = input.isHeldP2(Action::Run);
            pi.runPressed   = input.isJustPressedP2(Action::Run);
        }
        return pi;
    }
};

} // anonymous namespace

void PlayerSystem::update(entt::registry& reg, float dt,
                           const InputManager& input, EventBus& events) {
    auto view = reg.view<PlayerComponent, TransformComponent, VelocityComponent, HealthComponent>();

    for (auto entity : view) {
        auto& player = view.get<PlayerComponent>(entity);
        auto& vel    = view.get<VelocityComponent>(entity);
        auto& health = view.get<HealthComponent>(entity);

        // ================================================================
        // Tick invincibility frames
        // ================================================================
        if (health.invincibilityFrames > 0) {
            --health.invincibilityFrames;
        }

        // ================================================================
        // Dead — no input, no movement
        // ================================================================
        if (health.isDead) {
            player.state = PlayerState::Dead;
            vel.velocity.x = 0.0f;
            continue;
        }

        // ================================================================
        // Growing / Shrinking animation — freeze movement
        // ================================================================
        if (player.state == PlayerState::Growing || player.state == PlayerState::Shrinking) {
            player.growTimer -= dt;
            vel.velocity = {0.0f, 0.0f};
            if (player.growTimer <= 0.0f) {
                player.state = PlayerState::Idle;
            }
            continue;
        }

        // ================================================================
        // FlagPole sliding — auto-controlled
        // ================================================================
        if (player.state == PlayerState::FlagPole) {
            vel.velocity.x = 0.0f;
            vel.velocity.y = 200.0f; // slide down
            continue;
        }

        // ================================================================
        // Entering pipe — auto-controlled
        // ================================================================
        if (player.state == PlayerState::EnteringPipe) {
            player.pipeTimer -= dt;
            vel.velocity.x = 0.0f;
            vel.velocity.y = 60.0f; // sink into pipe
            if (player.pipeTimer <= 0.0f) {
                auto& transform = view.get<TransformComponent>(entity);
                transform.position = player.pipeTarget;
                player.state = PlayerState::Idle;
                vel.velocity.y = 0.0f;
            }
            continue;
        }

        // ================================================================
        // Hurt — wait for i-frames to expire before returning to normal
        // ================================================================
        if (player.state == PlayerState::Hurt) {
            if (health.invincibilityFrames <= 0) {
                player.state = player.isGrounded ? PlayerState::Idle : PlayerState::Falling;
            }
            continue;
        }

        // ================================================================
        // Read input for this player
        // ================================================================
        auto pi = PlayerInput::read(input, player.playerIndex);

        // ================================================================
        // Timer updates
        // ================================================================
        player.jumpBufferTimer = std::max(0.0f, player.jumpBufferTimer - dt);

        // Coyote time: refresh while grounded, count down when airborne
        if (player.isGrounded && player.jumpCount == 0) {
            player.coyoteTimer = PlayerComponent::COYOTE_TIME;
        } else if (!player.isGrounded) {
            player.coyoteTimer = std::max(0.0f, player.coyoteTimer - dt);
        }

        // ================================================================
        // Buffer jump input
        // ================================================================
        if (pi.jumpPressed) {
            player.jumpBufferTimer = PlayerComponent::JUMP_BUFFER;
        }

        // ================================================================
        // Facing direction
        // ================================================================
        if (pi.moveX != 0.0f) {
            player.facing = (pi.moveX > 0.0f) ? 1 : -1;
        }

        player.isRunning = pi.runHeld;
        bool wasGrounded = player.isGrounded;

        // ================================================================
        // JUMP
        // ================================================================
        bool canGroundJump = player.isGrounded || player.coyoteTimer > 0.0f;

        if (canGroundJump && player.jumpBufferTimer > 0.0f && player.jumpCount == 0) {
            player.state = PlayerState::Jumping;
            vel.velocity.y = Config::PLAYER_JUMP_FORCE;
            player.jumpCount = 1;
            player.jumpBufferTimer = 0.0f;
            player.coyoteTimer = 0.0f;
            events.publish(PlayerJumpEvent{1, player.playerIndex});
        }

        // Variable jump height: release jump early → cut upward velocity
        if (pi.jumpReleased && vel.velocity.y < 0.0f) {
            vel.velocity.y *= Config::PLAYER_JUMP_CUT;
        }

        // ================================================================
        // FIREBALL (Fire Mario only, on Run press)
        // ================================================================
        if (pi.runPressed && player.power == MarioPowerState::Fire) {
            // Spawn fireball
            auto& transform = view.get<TransformComponent>(entity);
            Vec2f fbPos = {
                transform.position.x + static_cast<float>(player.facing) * 20.0f,
                transform.position.y + 8.0f
            };

            auto fireball = reg.create();
            reg.emplace<TransformComponent>(fireball, TransformComponent{fbPos});
            reg.emplace<VelocityComponent>(fireball, VelocityComponent{
                {static_cast<float>(player.facing) * Config::FIREBALL_SPEED, 0.0f}
            });
            reg.emplace<GravityComponent>(fireball, GravityComponent{
                Config::FIREBALL_GRAVITY / Config::GRAVITY
            });
            reg.emplace<ColliderComponent>(fireball, ColliderComponent{
                {0.0f, 0.0f}, {8.0f, 8.0f}, true, false
            });
            reg.emplace<ProjectileComponent>(fireball, ProjectileComponent{
                static_cast<uint32_t>(entt::to_integral(entity)),
                1, Config::FIREBALL_LIFETIME, Config::FIREBALL_SPEED,
                {static_cast<float>(player.facing), 0.0f},
                true, false
            });

            SpriteComponent fbSprite;
            auto fbTex = ResourceManager::instance().getTexture("fireball");
            fbSprite.texture = fbTex.value_or(TextureHandle{0});
            fbSprite.srcRect = {0.0f, 0.0f, 8.0f, 8.0f};
            fbSprite.zOrder = 8;
            reg.emplace<SpriteComponent>(fireball, fbSprite);
            reg.emplace<TagComponent>(fireball, TagComponent{"fireball"});

            events.publish(FireballEvent{fbPos, player.playerIndex});
        }

        // ================================================================
        // HORIZONTAL MOVEMENT (acceleration-based, Mario-style)
        // ================================================================
        float maxSpeed = player.isRunning ? Config::PLAYER_RUN_SPEED : Config::PLAYER_WALK_SPEED;
        float accel = player.isGrounded ? Config::PLAYER_ACCEL : Config::PLAYER_AIR_ACCEL;

        if (pi.moveX != 0.0f) {
            float targetSpeed = pi.moveX * maxSpeed;
            // Accelerate toward target
            vel.velocity.x = Math::approach(vel.velocity.x, targetSpeed, accel * dt);
        } else if (player.isGrounded) {
            // Decelerate on ground when no input
            vel.velocity.x = Math::approach(vel.velocity.x, 0.0f, Config::PLAYER_DECEL * dt);
        }
        // In air with no input: retain momentum (no air friction)

        // Clamp to max speed
        if (std::abs(vel.velocity.x) > maxSpeed) {
            vel.velocity.x = Math::clamp(vel.velocity.x, -maxSpeed, maxSpeed);
        }

        // ================================================================
        // DETERMINE DISPLAY STATE
        // ================================================================
        if (player.state != PlayerState::Hurt &&
            player.state != PlayerState::Growing &&
            player.state != PlayerState::Shrinking &&
            player.state != PlayerState::FlagPole &&
            player.state != PlayerState::EnteringPipe) {

            if (player.isGrounded) {
                player.jumpCount = 0;

                if (!wasGrounded) {
                    events.publish(PlayerLandedEvent{player.playerIndex});
                }

                // Skidding: moving one direction but facing the other with speed
                bool skidding = (pi.moveX != 0.0f) &&
                    ((vel.velocity.x > 50.0f && pi.moveX < 0.0f) ||
                     (vel.velocity.x < -50.0f && pi.moveX > 0.0f));

                if (skidding) {
                    player.state = PlayerState::Skidding;
                } else if (std::abs(vel.velocity.x) > 10.0f) {
                    player.state = PlayerState::Running;
                } else {
                    player.state = PlayerState::Idle;
                }
            } else {
                if (vel.velocity.y < 0.0f) {
                    player.state = PlayerState::Jumping;
                } else {
                    player.state = PlayerState::Falling;
                }
            }
        }

        // Reset jump count when landing
        if (player.isGrounded) {
            player.jumpCount = 0;
        }
    }
}
