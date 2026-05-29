#include "ecs/systems/PlayerSystem.hpp"
#include "ecs/Components.hpp"
#include "core/InputManager.hpp"
#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "core/GameConfig.hpp"
#include "utils/Math.hpp"

#include <cmath>

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
        // Hurt — wait for i-frames to expire before returning to normal
        // ================================================================
        if (player.state == PlayerState::Hurt) {
            if (health.invincibilityFrames <= 0) {
                player.state = player.isGrounded ? PlayerState::Idle : PlayerState::Falling;
            }
            continue;
        }

        // ================================================================
        // Timer updates
        // ================================================================
        player.dashCooldownTimer = std::max(0.0f, player.dashCooldownTimer - dt);
        player.dashTimer         = std::max(0.0f, player.dashTimer - dt);
        player.jumpBufferTimer   = std::max(0.0f, player.jumpBufferTimer - dt);

        // Coyote time: refresh while grounded, count down when airborne
        if (player.isGrounded && player.jumpCount == 0) {
            player.coyoteTimer = PlayerComponent::COYOTE_TIME;
        } else if (!player.isGrounded) {
            player.coyoteTimer = std::max(0.0f, player.coyoteTimer - dt);
        }

        // ================================================================
        // Buffer jump input
        // ================================================================
        if (input.isJustPressed(Action::Jump)) {
            player.jumpBufferTimer = PlayerComponent::JUMP_BUFFER;
        }

        // ================================================================
        // Read horizontal input
        // ================================================================
        float moveX = 0.0f;
        if (input.isHeld(Action::MoveLeft))  moveX -= 1.0f;
        if (input.isHeld(Action::MoveRight)) moveX += 1.0f;

        if (moveX != 0.0f) {
            player.facing = (moveX > 0.0f) ? 1 : -1;
        }

        // ================================================================
        // DASHING — overrides everything else
        // ================================================================
        if (player.state == PlayerState::Dashing) {
            if (player.dashTimer <= 0.0f) {
                // Dash finished
                player.state = player.isGrounded ? PlayerState::Idle : PlayerState::Falling;
            } else {
                // During dash: fixed horizontal speed, no gravity (handled by PhysicsSystem)
                vel.velocity.x = Config::PLAYER_DASH_SPEED * static_cast<float>(player.facing);
                vel.velocity.y = 0.0f;
                continue;
            }
        }

        // Initiate dash
        if (input.isJustPressed(Action::Dash) &&
            player.dashCooldownTimer <= 0.0f &&
            player.state != PlayerState::Dead) {
            player.state            = PlayerState::Dashing;
            player.dashTimer        = Config::PLAYER_DASH_DURATION;
            player.dashCooldownTimer = Config::PLAYER_DASH_COOLDOWN;
            vel.velocity.x = Config::PLAYER_DASH_SPEED * static_cast<float>(player.facing);
            vel.velocity.y = 0.0f;
            continue;
        }

        // ================================================================
        // WALL SLIDING
        // ================================================================
        bool touchingWall = player.isTouchingWallLeft || player.isTouchingWallRight;
        bool pushingIntoWall =
            (player.isTouchingWallLeft  && input.isHeld(Action::MoveLeft)) ||
            (player.isTouchingWallRight && input.isHeld(Action::MoveRight));

        if (!player.isGrounded && touchingWall && pushingIntoWall && vel.velocity.y > 0.0f) {
            player.state = PlayerState::WallSliding;
            player.jumpCount = 1; // allow one jump (wall jump)
        } else if (player.state == PlayerState::WallSliding) {
            // Left wall slide state — either jumped, released, or landed
            player.state = player.isGrounded ? PlayerState::Idle : PlayerState::Falling;
        }

        // ================================================================
        // WALL JUMP
        // ================================================================
        if (player.state == PlayerState::WallSliding && player.jumpBufferTimer > 0.0f) {
            player.state = PlayerState::WallJumping;
            player.jumpBufferTimer = 0.0f;
            player.jumpCount = 1;

            // Kick away from wall
            int wallSide = player.isTouchingWallLeft ? -1 : 1;
            vel.velocity.x = Config::WALL_JUMP_FORCE_X * static_cast<float>(-wallSide);
            vel.velocity.y = Config::WALL_JUMP_FORCE_Y;
            player.facing = -wallSide;
            continue; // skip horizontal input this frame
        }

        // Brief lockout after wall jump (0.12s before air control resumes)
        if (player.state == PlayerState::WallJumping) {
            static constexpr float WALL_JUMP_LOCKOUT = 0.12f;
            // Estimate time since wall jump: if vy has risen enough, unlock
            // Simple approach: transition to jump/fall state after velocity reverses
            if (vel.velocity.y > 0.0f) {
                player.state = PlayerState::Falling;
            }
            // During lockout, don't apply horizontal input
            // (let the wall jump velocity carry)
            continue;
        }

        // ================================================================
        // GROUNDED JUMP / DOUBLE JUMP
        // ================================================================
        bool canGroundJump = player.isGrounded || player.coyoteTimer > 0.0f;

        if (canGroundJump && player.jumpBufferTimer > 0.0f && player.jumpCount == 0) {
            // Ground jump (or coyote jump)
            player.state = PlayerState::Jumping;
            vel.velocity.y = Config::PLAYER_JUMP_FORCE;
            player.jumpCount = 1;
            player.jumpBufferTimer = 0.0f;
            player.coyoteTimer = 0.0f;
        }
        else if (!canGroundJump && player.jumpCount < Config::MAX_JUMPS &&
                 player.jumpBufferTimer > 0.0f) {
            // Double jump
            player.state = PlayerState::DoubleJumping;
            vel.velocity.y = Config::PLAYER_JUMP_FORCE;
            player.jumpCount = Config::MAX_JUMPS;
            player.jumpBufferTimer = 0.0f;
        }

        // Variable jump height: release jump early → cut upward velocity
        if (input.isJustReleased(Action::Jump) && vel.velocity.y < 0.0f) {
            vel.velocity.y *= 0.5f;
        }

        // ================================================================
        // HORIZONTAL MOVEMENT
        // ================================================================
        float targetSpeed = moveX * Config::PLAYER_SPEED;

        // Check for active speed boost power-up
        if (reg.all_of<PowerUpComponent>(entity)) {
            const auto& pu = reg.get<PowerUpComponent>(entity);
            if (pu.type == PowerUpType::SpeedBoost) {
                targetSpeed *= pu.speedMultiplier;
            }
        }

        vel.velocity.x = targetSpeed;

        // ================================================================
        // DETERMINE DISPLAY STATE (from velocity + grounded)
        // ================================================================
        if (player.state != PlayerState::Dashing &&
            player.state != PlayerState::WallSliding &&
            player.state != PlayerState::WallJumping &&
            player.state != PlayerState::Hurt) {

            if (player.isGrounded) {
                player.jumpCount = 0;
                if (std::abs(vel.velocity.x) > 1.0f) {
                    player.state = PlayerState::Running;
                } else {
                    player.state = PlayerState::Idle;
                }
            } else {
                if (vel.velocity.y < 0.0f) {
                    player.state = (player.jumpCount >= 2)
                        ? PlayerState::DoubleJumping
                        : PlayerState::Jumping;
                } else {
                    if (player.state != PlayerState::DoubleJumping || vel.velocity.y > 0.0f) {
                        player.state = PlayerState::Falling;
                    }
                }
            }
        }

        // Reset jump count when landing
        if (player.isGrounded) {
            player.jumpCount = 0;
        }
    }

    (void)events; // events used by collision system for player death/hurt
}
