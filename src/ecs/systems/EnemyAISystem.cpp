#include "ecs/systems/EnemyAISystem.hpp"
#include "ecs/Components.hpp"
#include "core/GameConfig.hpp"
#include "utils/Math.hpp"

#include <cmath>

void EnemyAISystem::update(entt::registry& reg, float dt) {
    // Find the player's position for sight-based AI
    Vec2f playerPos = {0.0f, 0.0f};
    bool  playerAlive = false;
    {
        auto pView = reg.view<PlayerComponent, TransformComponent, HealthComponent>();
        for (auto e : pView) {
            const auto& h = pView.get<HealthComponent>(e);
            if (!h.isDead) {
                playerPos = pView.get<TransformComponent>(e).position;
                playerAlive = true;
                break; // single player
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

        // -----------------------------------------------------------------
        // Per-type AI
        // -----------------------------------------------------------------

        switch (enemy.type) {

        // =================================================================
        // WALKER — patrol between patrol bounds, reverse at edges
        // =================================================================
        case EnemyType::Walker: {
            enemy.state = EnemyState::Patrol;

            float speed = 80.0f;
            vel.velocity.x = speed * static_cast<float>(enemy.facing);

            // Reverse at patrol bounds
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
        // JUMPER — same as walker but periodically bounces
        // =================================================================
        case EnemyType::Jumper: {
            enemy.state = EnemyState::Patrol;

            float speed = 60.0f;
            vel.velocity.x = speed * static_cast<float>(enemy.facing);

            // Reverse at patrol bounds
            if (transform.position.x <= enemy.patrolLeft) {
                transform.position.x = enemy.patrolLeft;
                enemy.facing = 1;
            } else if (transform.position.x >= enemy.patrolRight) {
                transform.position.x = enemy.patrolRight;
                enemy.facing = -1;
            }

            // Periodic bounce
            enemy.bounceTimer -= dt;
            if (enemy.bounceTimer <= 0.0f) {
                vel.velocity.y = enemy.bounceForce;
                enemy.bounceTimer = 1.5f + Math::randFloat(0.0f, 1.0f);
            }
            break;
        }

        // =================================================================
        // SHOOTER — patrol slowly, fire projectile when player is in sight
        // =================================================================
        case EnemyType::Shooter: {
            // Slow patrol
            float speed = 40.0f;
            vel.velocity.x = speed * static_cast<float>(enemy.facing);

            if (transform.position.x <= enemy.patrolLeft) {
                transform.position.x = enemy.patrolLeft;
                enemy.facing = 1;
            } else if (transform.position.x >= enemy.patrolRight) {
                transform.position.x = enemy.patrolRight;
                enemy.facing = -1;
            }

            enemy.state = EnemyState::Patrol;

            // Sight cone: horizontal line within ±48px vertical, 300px horizontal range
            if (playerAlive) {
                float dx = playerPos.x - transform.position.x;
                float dy = playerPos.y - transform.position.y;
                bool inVerticalRange  = std::abs(dy) < 48.0f;
                bool inHorizontalRange = std::abs(dx) < 300.0f;
                bool facingPlayer = (dx > 0.0f && enemy.facing == 1) ||
                                    (dx < 0.0f && enemy.facing == -1);

                if (inVerticalRange && inHorizontalRange && facingPlayer) {
                    enemy.isAlerted = true;
                    enemy.state = EnemyState::Attack;
                    vel.velocity.x = 0.0f; // stop moving while shooting

                    enemy.shootCooldown -= dt;
                    if (enemy.shootCooldown <= 0.0f) {
                        enemy.shootCooldown = enemy.shootInterval;

                        // Spawn projectile entity
                        auto projEntity = reg.create();
                        float projDir = (enemy.facing == 1) ? 1.0f : -1.0f;
                        Vec2f spawnPos = {
                            transform.position.x + projDir * 20.0f,
                            transform.position.y + 8.0f
                        };

                        reg.emplace<TransformComponent>(projEntity,
                            TransformComponent{spawnPos, {1.0f, 1.0f}, 0.0f});
                        reg.emplace<VelocityComponent>(projEntity,
                            VelocityComponent{{projDir * 250.0f, 0.0f}});
                        reg.emplace<ColliderComponent>(projEntity,
                            ColliderComponent{{0.0f, 0.0f}, {8.0f, 8.0f}, true, false});
                        reg.emplace<ProjectileComponent>(projEntity,
                            ProjectileComponent{
                                static_cast<uint32_t>(entity),
                                1,        // damage
                                3.0f,     // lifetime
                                250.0f,   // speed
                                {projDir, 0.0f}
                            });
                        reg.emplace<SpriteComponent>(projEntity); // placeholder sprite
                        reg.emplace<TagComponent>(projEntity, TagComponent{"projectile"});
                    }
                } else {
                    enemy.isAlerted = false;
                }
            }
            break;
        }

        // =================================================================
        // GUARDIAN — armored walker, flashes on first hit
        // =================================================================
        case EnemyType::Guardian: {
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

            // When armor is broken (isArmored == false), Guardian behaves like
            // a regular Walker but slower, vulnerable to one more hit.
            // The armor-break visual (flash) is handled by AnimationSystem
            // via the invincibilityFrames on HealthComponent.
            break;
        }

        } // switch
    }
}
