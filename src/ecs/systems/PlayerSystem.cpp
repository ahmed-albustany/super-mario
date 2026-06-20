#include "ecs/systems/PlayerSystem.hpp"
#include "ecs/Components.hpp"
#include "core/InputManager.hpp"
#include "core/PlayerController.hpp"
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
        // Dead — flip upside down, hop up, fall off screen, respawn delay
        // ================================================================
        if (health.isDead) {
            player.state = PlayerState::Dead;

            if (!player.deathAnimStarted) {
                // First frame of death: initiate death hop animation
                player.deathAnimStarted = true;
                player.deathTimer = Config::DEATH_RESPAWN_DELAY;
                vel.velocity.x = 0.0f;
                vel.velocity.y = Config::DEATH_HOP_FORCE;
                // Disable collision so player falls through everything
                if (reg.all_of<ColliderComponent>(entity)) {
                    reg.get<ColliderComponent>(entity).isTrigger = true;
                }
                // Flip sprite upside down
                if (reg.all_of<TransformComponent>(entity)) {
                    reg.get<TransformComponent>(entity).rotation = 180.0f;
                }
            }

            // During death: gravity pulls player down (handled by PhysicsSystem)
            vel.velocity.x = 0.0f;

            // Count down respawn timer
            player.deathTimer -= dt;
            continue;
        }

        // ================================================================
        // Hit state — brief invulnerability
        // ================================================================
        if (player.state == PlayerState::Hit) {
            if (health.invincibilityFrames <= 0) {
                player.state = player.isGrounded ? PlayerState::Idle : PlayerState::Falling;
            }
            continue;
        }

        // ================================================================
        // Appearing animation — freeze movement
        // ================================================================
        if (player.state == PlayerState::Appearing) {
            vel.velocity = {0.0f, 0.0f};
            continue;
        }

        // ================================================================
        // Read input for this player
        // ================================================================
        PlayerController pi(player.playerIndex);
        pi.update(input);

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
        if (pi.jumpPressed()) {
            player.jumpBufferTimer = PlayerComponent::JUMP_BUFFER;
        }

        // ================================================================
        // Facing direction
        // ================================================================
        if (pi.moveX() != 0.0f) {
            player.facing = (pi.moveX() > 0.0f) ? 1 : -1;
        }

        bool wasGrounded = player.isGrounded;

        // ================================================================
        // FIRST JUMP (ground jump + coyote time + jump buffer)
        // ================================================================
        bool canGroundJump = player.isGrounded || player.coyoteTimer > 0.0f;

        if (canGroundJump && player.jumpBufferTimer > 0.0f && player.jumpCount == 0) {
            player.state = PlayerState::Jumping;
            // Running jump gives extra height
            float jumpForce = Config::PLAYER_JUMP_FORCE;
            if (std::abs(vel.velocity.x) > Config::PLAYER_WALK_SPEED * 0.8f) {
                jumpForce *= 1.15f;
            }
            vel.velocity.y = jumpForce;
            player.jumpCount = 1;
            player.jumpBufferTimer = 0.0f;
            player.coyoteTimer = 0.0f;
            events.publish(PlayerJumpEvent{1, player.playerIndex});
        }

        // ================================================================
        // DOUBLE JUMP (X key / separate button, while airborne after first jump)
        // ================================================================
        if (pi.doubleJumpPressed() && player.jumpCount == 1 && !player.isGrounded) {
            player.state = PlayerState::DoubleJumping;
            vel.velocity.y = Config::PLAYER_DOUBLE_JUMP_FORCE;
            player.jumpCount = 2;
            events.publish(PlayerJumpEvent{2, player.playerIndex});
        }

        // Variable jump height: release jump early → cut upward velocity
        if (pi.jumpReleased() && vel.velocity.y < 0.0f) {
            vel.velocity.y *= Config::PLAYER_JUMP_CUT;
        }

        // ================================================================
        // HORIZONTAL MOVEMENT (acceleration-based)
        // ================================================================
        float maxSpeed = Config::PLAYER_WALK_SPEED;
        float accel = player.isGrounded ? Config::PLAYER_ACCEL : Config::PLAYER_AIR_ACCEL;

        if (pi.moveX() != 0.0f) {
            float targetSpeed = pi.moveX() * maxSpeed;
            vel.velocity.x = Math::approach(vel.velocity.x, targetSpeed, accel * dt);
        } else if (player.isGrounded) {
            vel.velocity.x = Math::approach(vel.velocity.x, 0.0f, Config::PLAYER_DECEL * dt);
        }

        // Clamp to max speed
        if (std::abs(vel.velocity.x) > maxSpeed) {
            vel.velocity.x = Math::clamp(vel.velocity.x, -maxSpeed, maxSpeed);
        }

        // ================================================================
        // DETERMINE DISPLAY STATE
        // ================================================================
        if (player.state != PlayerState::Hit &&
            player.state != PlayerState::Appearing &&
            player.state != PlayerState::Disappearing) {

            if (player.isGrounded) {
                player.jumpCount = 0;

                if (!wasGrounded) {
                    events.publish(PlayerLandedEvent{player.playerIndex});
                }

                if (std::abs(vel.velocity.x) > 10.0f) {
                    player.state = PlayerState::Running;
                } else {
                    player.state = PlayerState::Idle;
                }
            } else {
                if (player.jumpCount >= 2) {
                    player.state = PlayerState::DoubleJumping;
                } else if (vel.velocity.y < 0.0f) {
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
