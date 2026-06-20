#include "ecs/systems/AnimationSystem.hpp"
#include "ecs/Components.hpp"

#include <cmath>
#include <string>

namespace {

/// @brief Map PlayerState to animation clip name for Pixel Adventure characters.
const char* playerStateToClip(PlayerState state) {
    switch (state) {
        case PlayerState::Idle:           return "idle";
        case PlayerState::Running:        return "run";
        case PlayerState::Jumping:        return "jump";
        case PlayerState::DoubleJumping:  return "double_jump";
        case PlayerState::Falling:        return "fall";
        case PlayerState::WallJumping:    return "wall_jump";
        case PlayerState::Hit:           return "hit";
        case PlayerState::Dead:           return "hit";
        case PlayerState::Appearing:      return "appearing";
        case PlayerState::Disappearing:   return "disappearing";
    }
    return "idle";
}

/// @brief Map EnemyState enum to animation clip name (legacy compat).
const char* enemyStateToClip(EnemyState state) {
    switch (state) {
        case EnemyState::Idle:    return "idle";
        case EnemyState::Patrol:  return "walk";
        case EnemyState::Chase:   return "walk";
        case EnemyState::Attack:  return "attack";
        case EnemyState::Shell:   return "shell";
        case EnemyState::Hurt:    return "hurt";
        case EnemyState::Dead:    return "dead";
    }
    return "idle";
}

} // anonymous namespace

void AnimationSystem::update(entt::registry& reg, float dt) {
    // ---- Map player state to animation clip ----
    {
        auto view = reg.view<PlayerComponent, AnimationComponent, SpriteComponent>();
        for (auto entity : view) {
            const auto& player = view.get<PlayerComponent>(entity);
            auto& anim         = view.get<AnimationComponent>(entity);
            auto& sprite       = view.get<SpriteComponent>(entity);

            anim.play(playerStateToClip(player.state));
            sprite.flipX = (player.facing < 0);
        }
    }

    // ---- Enemy animation selection (legacy compat) ----
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

            // Bounds-check currentClip
            if (anim.currentClip < 0 || anim.currentClip >= static_cast<int>(anim.clips.size())) {
                anim.currentClip = 0;
            }
            auto& clip = anim.clips[static_cast<size_t>(anim.currentClip)];
            if (clip.frames.empty()) continue;

            if (anim.finished && !clip.loop) {
                if (clip.texture.valid()) sprite.texture = clip.texture;
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

            // Swap texture if the clip specifies one
            if (clip.texture.valid()) {
                sprite.texture = clip.texture;
            }

            // Update sprite source rect to current animation frame
            size_t frameIdx = static_cast<size_t>(
                std::max(0, std::min(anim.currentFrame, static_cast<int>(clip.frames.size()) - 1)));
            sprite.srcRect = clip.frames[frameIdx];
        }
    }

    // ---- Invincibility blink effect ----
    {
        auto view = reg.view<HealthComponent, SpriteComponent>();
        for (auto entity : view) {
            const auto& health = view.get<HealthComponent>(entity);
            auto& sprite       = view.get<SpriteComponent>(entity);

            if (health.invincibilityFrames > 0) {
                sprite.visible = ((health.invincibilityFrames / 4) % 2 == 0);
            } else {
                sprite.visible = true;
            }
        }
    }

    // ---- Floating text lifetime + rise ----
    {
        auto view = reg.view<FloatingTextComponent, TransformComponent>();
        for (auto entity : view) {
            auto& ft  = view.get<FloatingTextComponent>(entity);
            auto& pos = view.get<TransformComponent>(entity);
            ft.elapsed += dt;
            pos.position.y -= ft.riseSpeed * dt;
            if (ft.elapsed >= ft.lifetime) {
                reg.emplace_or_replace<DestroyFlag>(entity);
            }
        }
    }

    // ---- Particle emitter lifetime ----
    {
        auto view = reg.view<ParticleEmitterComponent>();
        for (auto entity : view) {
            auto& pe = view.get<ParticleEmitterComponent>(entity);
            pe.elapsed += dt;
            if (pe.elapsed >= pe.lifetime) {
                reg.emplace_or_replace<DestroyFlag>(entity);
            }
        }
    }
}
