#include "ecs/systems/EnemyAISystem.hpp"
#include "ecs/Components.hpp"
#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "core/GameConfig.hpp"
#include "core/ResourceManager.hpp"
#include "utils/Math.hpp"

#include <cmath>

void EnemyAISystem::update(entt::registry& reg, float dt, EventBus& events) {
    // Find the nearest living player position for AI targeting
    Vec2f playerPos = {0.0f, 0.0f};
    bool  playerAlive = false;
    {
        auto pView = reg.view<PlayerComponent, TransformComponent, HealthComponent>();
        float bestDist = 99999.0f;
        for (auto e : pView) {
            const auto& h = pView.get<HealthComponent>(e);
            if (!h.isDead) {
                Vec2f pos = pView.get<TransformComponent>(e).position;
                // Use first alive player (or nearest in co-op)
                if (!playerAlive) {
                    playerPos = pos;
                    playerAlive = true;
                    bestDist = pos.lengthSq();
                } else {
                    float d = pos.lengthSq();
                    if (d < bestDist) {
                        playerPos = pos;
                        bestDist = d;
                    }
                }
            }
        }
    }

    auto view = reg.view<EnemyComponent, TransformComponent, VelocityComponent, HealthComponent>();

    for (auto entity : view) {
        auto& enemy     = view.get<EnemyComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);
        auto& vel       = view.get<VelocityComponent>(entity);
        auto& health    = view.get<HealthComponent>(entity);

        // Dead enemy — no AI
        if (health.isDead || enemy.state == EnemyState::Dead) {
            vel.velocity.x = 0.0f;
            continue;
        }

        // Tick invincibility
        if (health.invincibilityFrames > 0) {
            --health.invincibilityFrames;
        }

        // Koopa shell sliding — separate logic
        if (enemy.state == EnemyState::Shell) {
            if (enemy.shellMoving) {
                vel.velocity.x = enemy.shellSpeed * static_cast<float>(enemy.facing);
            } else {
                vel.velocity.x = 0.0f;
            }
            // Reverse at patrol bounds
            if (transform.position.x <= enemy.patrolLeft) {
                enemy.facing = 1;
            } else if (transform.position.x >= enemy.patrolRight) {
                enemy.facing = -1;
            }
            continue;
        }

        switch (enemy.type) {

        // =================================================================
        // GOOMBA — walk, reverse at edges, die on stomp
        // =================================================================
        case EnemyType::Goomba: {
            enemy.state = EnemyState::Patrol;
            float speed = 60.0f;
            vel.velocity.x = speed * static_cast<float>(enemy.facing);

            if (transform.position.x <= enemy.patrolLeft) {
                transform.position.x = enemy.patrolLeft;
                enemy.facing = 1;
            } else if (transform.position.x >= enemy.patrolRight) {
                transform.position.x = enemy.patrolRight;
                enemy.facing = -1;
            }
            break;
        }

        // =================================================================
        // KOOPA — walk, reverse at edges, stomp → shell
        // =================================================================
        case EnemyType::Koopa: {
            enemy.state = EnemyState::Patrol;
            float speed = 50.0f;
            vel.velocity.x = speed * static_cast<float>(enemy.facing);

            if (transform.position.x <= enemy.patrolLeft) {
                transform.position.x = enemy.patrolLeft;
                enemy.facing = 1;
            } else if (transform.position.x >= enemy.patrolRight) {
                transform.position.x = enemy.patrolRight;
                enemy.facing = -1;
            }
            break;
        }

        // =================================================================
        // PIRANHA PLANT — bobs up/down from pipe, can't be stomped
        // =================================================================
        case EnemyType::PiranhaPlant: {
            enemy.state = EnemyState::Idle;
            vel.velocity.x = 0.0f;

            // Bob cycle: 0..1 over ~3 seconds
            enemy.bobTimer += dt;
            float cycle = 3.0f;
            enemy.bobPhase = std::fmod(enemy.bobTimer, cycle) / cycle;

            // Rise up for first half, retreat for second half
            float bobHeight = 32.0f;
            float offset;
            if (enemy.bobPhase < 0.3f) {
                // Rising
                offset = -bobHeight * (enemy.bobPhase / 0.3f);
            } else if (enemy.bobPhase < 0.7f) {
                // Fully out
                offset = -bobHeight;
            } else {
                // Retreating
                offset = -bobHeight * (1.0f - (enemy.bobPhase - 0.7f) / 0.3f);
            }
            transform.position.y = enemy.bobBaseY + offset;

            // Don't alert if player is very close to pipe top (classic behavior)
            break;
        }

        // =================================================================
        // BOWSER — patrol, shoot fire, takes multiple hits
        // =================================================================
        case EnemyType::Bowser: {
            enemy.state = EnemyState::Patrol;
            float speed = 40.0f;
            vel.velocity.x = speed * static_cast<float>(enemy.facing);

            if (transform.position.x <= enemy.patrolLeft) {
                transform.position.x = enemy.patrolLeft;
                enemy.facing = 1;
            } else if (transform.position.x >= enemy.patrolRight) {
                transform.position.x = enemy.patrolRight;
                enemy.facing = -1;
            }

            // Fire breath at player
            if (playerAlive) {
                float dx = playerPos.x - transform.position.x;
                bool facingPlayer = (dx > 0.0f && enemy.facing == 1) ||
                                    (dx < 0.0f && enemy.facing == -1);

                if (facingPlayer && std::abs(dx) < 400.0f) {
                    enemy.state = EnemyState::Attack;

                    enemy.shootCooldown -= dt;
                    if (enemy.shootCooldown <= 0.0f) {
                        enemy.shootCooldown = enemy.shootInterval;

                        float projDir = (enemy.facing == 1) ? 1.0f : -1.0f;
                        Vec2f spawnPos = {
                            transform.position.x + projDir * 24.0f,
                            transform.position.y + 8.0f
                        };

                        auto projEntity = reg.create();
                        reg.emplace<TransformComponent>(projEntity,
                            TransformComponent{spawnPos, {1.0f, 1.0f}, 0.0f});
                        reg.emplace<VelocityComponent>(projEntity,
                            VelocityComponent{{projDir * 200.0f, 0.0f}});
                        reg.emplace<ColliderComponent>(projEntity,
                            ColliderComponent{{0.0f, 0.0f}, {12.0f, 8.0f}, true, false});
                        reg.emplace<ProjectileComponent>(projEntity,
                            ProjectileComponent{
                                static_cast<uint32_t>(entity),
                                1, 4.0f, 200.0f,
                                {projDir, 0.0f},
                                false, true
                            });

                        SpriteComponent projSprite;
                        auto projTex = ResourceManager::instance().getTexture("bowser_fire");
                        projSprite.texture = projTex.value_or(TextureHandle{0});
                        projSprite.srcRect = {0.0f, 0.0f, 16.0f, 8.0f};
                        projSprite.zOrder = 8;
                        reg.emplace<SpriteComponent>(projEntity, projSprite);
                        reg.emplace<TagComponent>(projEntity, TagComponent{"bowser_fire"});

                        events.publish(EnemyShootEvent{spawnPos});
                    }
                }
            }
            break;
        }

        } // switch
    }
}
