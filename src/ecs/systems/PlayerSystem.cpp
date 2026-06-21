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
                player.deathAnimStarted = true;
                player.deathTimer = Config::DEATH_RESPAWN_DELAY;
                vel.velocity.x = 0.0f;
                vel.velocity.y = Config::DEATH_HOP_FORCE;
                if (reg.all_of<ColliderComponent>(entity)) {
                    reg.get<ColliderComponent>(entity).isTrigger = true;
                }
                if (reg.all_of<TransformComponent>(entity)) {
                    reg.get<TransformComponent>(entity).rotation = 180.0f;
                }
            }

            vel.velocity.x = 0.0f;
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
        bool isRunning = pi.runHeld();

        // ================================================================
        // FIRST JUMP (ground jump + coyote time + jump buffer)
        // ================================================================
        bool canGroundJump = player.isGrounded || player.coyoteTimer > 0.0f;

        if (canGroundJump && player.jumpBufferTimer > 0.0f && player.jumpCount == 0) {
            player.state = PlayerState::Jumping;
            float jumpForce = Config::PLAYER_JUMP_FORCE;
            // Running jump: higher and farther
            if (isRunning && std::abs(vel.velocity.x) > Config::PLAYER_WALK_SPEED * 0.5f) {
                jumpForce *= Config::PLAYER_RUN_JUMP_MULT;
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

        // ================================================================
        // VARIABLE JUMP HEIGHT + ASYMMETRIC GRAVITY
        // Mario-style: holding jump = floaty rise, releasing = fast fall
        // ================================================================
        if (vel.velocity.y < 0.0f) {
            // Rising — if jump released early, apply stronger gravity to cut the jump short
            if (!pi.jumpHeld() && player.jumpCount > 0) {
                vel.velocity.y += Config::GRAVITY * (Config::PLAYER_LOW_JUMP_MULT - 1.0f) * dt;
            }
        } else if (vel.velocity.y > 0.0f) {
            // Falling — apply extra gravity for snappy, weighty feel
            vel.velocity.y += Config::GRAVITY * (Config::PLAYER_FALL_MULT - 1.0f) * dt;
        }

        // ================================================================
        // HORIZONTAL MOVEMENT (acceleration-based, Mario-style)
        // Hold direction: gradual speed build. Release: snappy ground stop, drifty air.
        // Hold run key: higher max speed.
        // ================================================================
        float maxSpeed = isRunning ? Config::PLAYER_RUN_SPEED : Config::PLAYER_WALK_SPEED;

        if (pi.moveX() != 0.0f) {
            float targetSpeed = pi.moveX() * maxSpeed;
            float accel = player.isGrounded ? Config::PLAYER_ACCEL : Config::PLAYER_AIR_ACCEL;

            // Skid: reversing direction while moving uses decel for snappier turnaround
            bool skidding = (vel.velocity.x > 0.0f && pi.moveX() < 0.0f) ||
                            (vel.velocity.x < 0.0f && pi.moveX() > 0.0f);
            if (skidding && player.isGrounded) {
                accel = Config::PLAYER_DECEL;
            }

            vel.velocity.x = Math::approach(vel.velocity.x, targetSpeed, accel * dt);
        } else {
            // No input: decelerate
            float decel = player.isGrounded ? Config::PLAYER_DECEL : Config::PLAYER_AIR_DECEL;
            vel.velocity.x = Math::approach(vel.velocity.x, 0.0f, decel * dt);
        }

        // If over max speed (e.g., was running, released run key), let speed decay naturally
        // instead of hard-clamping — feels more like Mario
        if (std::abs(vel.velocity.x) > maxSpeed && pi.moveX() != 0.0f) {
            float sign = vel.velocity.x > 0.0f ? 1.0f : -1.0f;
            vel.velocity.x = Math::approach(vel.velocity.x, sign * maxSpeed,
                                             Config::PLAYER_DECEL * 0.5f * dt);
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
