#include "ecs/systems/AnimationSystem.hpp"
#include "ecs/Components.hpp"

#include <cmath>
#include <string>

namespace {

/// @brief Map PlayerState enum to animation clip name.
const char* playerStateToClip(PlayerState state) {
    switch (state) {
        case PlayerState::Idle:           return "idle";
        case PlayerState::Running:        return "run";
        case PlayerState::Jumping:        return "jump";
        case PlayerState::DoubleJumping:  return "double_jump";
        case PlayerState::Falling:        return "fall";
        case PlayerState::Dashing:        return "dash";
        case PlayerState::WallSliding:    return "wall_slide";
        case PlayerState::WallJumping:    return "wall_jump";
        case PlayerState::Hurt:           return "hurt";
        case PlayerState::Dead:           return "dead";
    }
    return "idle";
}

/// @brief Map EnemyState enum to animation clip name.
const char* enemyStateToClip(EnemyState state) {
    switch (state) {
        case EnemyState::Idle:    return "idle";
        case EnemyState::Patrol:  return "walk";
        case EnemyState::Chase:   return "walk";
        case EnemyState::Attack:  return "attack";
        case EnemyState::Hurt:    return "hurt";
        case EnemyState::Dead:    return "dead";
    }
    return "idle";
}

} // anonymous namespace

void AnimationSystem::update(entt::registry& reg, float dt) {
    // ---- Map entity state to animation clip name ----

    // Player animation selection
    {
        auto view = reg.view<PlayerComponent, AnimationComponent, SpriteComponent>();
        for (auto entity : view) {
            const auto& player = view.get<PlayerComponent>(entity);
            auto& anim         = view.get<AnimationComponent>(entity);
            auto& sprite       = view.get<SpriteComponent>(entity);

            // Set clip based on state
            anim.play(playerStateToClip(player.state));

            // Flip sprite based on facing direction
            sprite.flipX = (player.facing < 0);
        }
    }

    // Enemy animation selection
    {
        auto view = reg.view<EnemyComponent, AnimationComponent, SpriteComponent>();
        for (auto entity : view) {
            const auto& enemy = view.get<EnemyComponent>(entity);
            auto& anim        = view.get<AnimationComponent>(entity);
            auto& sprite      = view.get<SpriteComponent>(entity);

            anim.play(enemyStateToClip(enemy.state));
            sprite.flipX = (enemy.facing < 0);
        }
    }

    // ---- Advance animation frames for ALL animated entities ----
    {
        auto view = reg.view<AnimationComponent, SpriteComponent>();
        for (auto entity : view) {
            auto& anim   = view.get<AnimationComponent>(entity);
            auto& sprite = view.get<SpriteComponent>(entity);

            if (anim.clips.empty()) continue;

            auto& clip = anim.clips[static_cast<size_t>(anim.currentClip)];
            if (clip.frames.empty()) continue;

            if (anim.finished && !clip.loop) {
                // Non-looping clip has ended — stay on last frame
                sprite.srcRect = clip.frames.back();
                continue;
            }

            anim.frameTimer += dt;
            float frameDuration = 1.0f / clip.fps;

            while (anim.frameTimer >= frameDuration) {
                anim.frameTimer -= frameDuration;
                anim.currentFrame++;

                if (anim.currentFrame >= static_cast<int>(clip.frames.size())) {
                    if (clip.loop) {
                        anim.currentFrame = 0;
                    } else {
                        anim.currentFrame = static_cast<int>(clip.frames.size()) - 1;
                        anim.finished = true;
                        break;
                    }
                }
            }

            // Update sprite source rect to current animation frame
            sprite.srcRect = clip.frames[static_cast<size_t>(anim.currentFrame)];
        }
    }

    // ---- Invincibility blink effect ----
    {
        auto view = reg.view<HealthComponent, SpriteComponent>();
        for (auto entity : view) {
            const auto& health = view.get<HealthComponent>(entity);
            auto& sprite       = view.get<SpriteComponent>(entity);

            if (health.invincibilityFrames > 0) {
                // Blink: toggle visibility every 4 frames
                sprite.visible = ((health.invincibilityFrames / 4) % 2 == 0);
            } else {
                sprite.visible = true;
            }
        }
    }
}
