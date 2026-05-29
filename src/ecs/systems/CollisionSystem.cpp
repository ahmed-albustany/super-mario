#include "ecs/systems/CollisionSystem.hpp"
#include "ecs/Components.hpp"
#include "core/GameConfig.hpp"
#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "physics/AABB.hpp"
#include "utils/Logger.hpp"

#include <algorithm>
#include <cmath>
#include <set>

// =============================================================================
// Anonymous-namespace helpers — MUST be defined before class methods that call them
// =============================================================================

namespace {

void handlePlayerEnemyCollision(entt::registry& reg, EventBus& events,
                                 entt::entity playerEnt, entt::entity enemyEnt,
                                 const Rect& playerRect, const Rect& enemyRect) {
    auto& player  = reg.get<PlayerComponent>(playerEnt);
    auto& pHealth = reg.get<HealthComponent>(playerEnt);
    auto& pVel    = reg.get<VelocityComponent>(playerEnt);

    // Invincible player (power-up or i-frames) → kill enemy on contact
    if (pHealth.invincibilityFrames > 0) {
        if (reg.all_of<HealthComponent>(enemyEnt)) {
            auto& eHealth = reg.get<HealthComponent>(enemyEnt);
            eHealth.hp = 0;
            eHealth.isDead = true;
            if (reg.all_of<EnemyComponent>(enemyEnt)) {
                reg.get<EnemyComponent>(enemyEnt).state = EnemyState::Dead;
            }
            events.publish(EnemyKilledEvent{"enemy",
                reg.get<TransformComponent>(enemyEnt).position});
            reg.emplace_or_replace<DestroyFlag>(enemyEnt);
        }
        return;
    }

    // Dashing player → kill enemy (unless Guardian with armor)
    if (player.state == PlayerState::Dashing) {
        if (reg.all_of<EnemyComponent>(enemyEnt)) {
            auto& ec = reg.get<EnemyComponent>(enemyEnt);
            if (ec.type == EnemyType::Guardian && ec.isArmored) {
                ec.isArmored = false;
                if (reg.all_of<HealthComponent>(enemyEnt)) {
                    reg.get<HealthComponent>(enemyEnt).invincibilityFrames = 30;
                }
                return;
            }
        }
        if (reg.all_of<HealthComponent>(enemyEnt)) {
            auto& eH = reg.get<HealthComponent>(enemyEnt);
            eH.hp = 0;
            eH.isDead = true;
        }
        if (reg.all_of<EnemyComponent>(enemyEnt)) {
            reg.get<EnemyComponent>(enemyEnt).state = EnemyState::Dead;
        }
        events.publish(EnemyKilledEvent{"enemy",
            reg.get<TransformComponent>(enemyEnt).position});
        reg.emplace_or_replace<DestroyFlag>(enemyEnt);
        return;
    }

    // Stomp detection: player's bottom is above enemy's vertical midpoint
    float playerBottom = playerRect.bottom();
    float enemyMidY    = enemyRect.y + enemyRect.h * 0.5f;

    if (playerBottom <= enemyMidY && pVel.velocity.y > 0.0f) {
        // Stomp — bounce player up
        pVel.velocity.y = Config::PLAYER_JUMP_FORCE * 0.7f;

        if (reg.all_of<EnemyComponent>(enemyEnt)) {
            auto& ec = reg.get<EnemyComponent>(enemyEnt);
            if (ec.type == EnemyType::Guardian && ec.isArmored) {
                ec.isArmored = false;
                if (reg.all_of<HealthComponent>(enemyEnt)) {
                    reg.get<HealthComponent>(enemyEnt).invincibilityFrames = 30;
                }
            } else {
                if (reg.all_of<HealthComponent>(enemyEnt)) {
                    auto& eH = reg.get<HealthComponent>(enemyEnt);
                    eH.hp = 0;
                    eH.isDead = true;
                }
                ec.state = EnemyState::Dead;
                events.publish(EnemyKilledEvent{"enemy",
                    reg.get<TransformComponent>(enemyEnt).position});
                reg.emplace_or_replace<DestroyFlag>(enemyEnt);
            }
        }
    } else {
        // Side/below contact — player takes damage
        if (pHealth.invincibilityFrames <= 0) {
            pHealth.hp -= 1;
            pHealth.invincibilityFrames = Config::INVINCIBILITY_FRAMES;
            player.state = PlayerState::Hurt;

            float knockDir = (playerRect.center().x < enemyRect.center().x) ? -1.0f : 1.0f;
            pVel.velocity.x = knockDir * 200.0f;
            pVel.velocity.y = -300.0f;

            if (pHealth.hp <= 0) {
                pHealth.isDead = true;
                events.publish(PlayerDiedEvent{0});
            } else {
                events.publish(PlayerHurtEvent{pHealth.hp});
            }
        }
    }
}

void handlePlayerCollectible(entt::registry& reg, EventBus& events,
                              entt::entity collectEnt) {
    auto& collectible = reg.get<CollectibleComponent>(collectEnt);
    if (collectible.collected) return;

    collectible.collected = true;
    Vec2f pos = reg.get<TransformComponent>(collectEnt).position;

    switch (collectible.type) {
        case CollectibleType::Coin:
            events.publish(CoinCollectedEvent{collectible.value, pos});
            break;
        case CollectibleType::GemShard:
            events.publish(GemCollectedEvent{collectible.value, pos});
            break;
        case CollectibleType::PowerCrystal:
            events.publish(PowerUpActivatedEvent{"invincibility", Config::POWER_CRYSTAL_DURATION});
            break;
    }

    reg.emplace_or_replace<DestroyFlag>(collectEnt);
}

void handleProjectileHit(entt::registry& reg, EventBus& events,
                          entt::entity projEnt, entt::entity targetEnt) {
    const auto& proj = reg.get<ProjectileComponent>(projEnt);

    if (static_cast<uint32_t>(targetEnt) == proj.ownerId) return;

    if (reg.all_of<HealthComponent>(targetEnt)) {
        auto& health = reg.get<HealthComponent>(targetEnt);

        if (health.invincibilityFrames > 0) {
            reg.emplace_or_replace<DestroyFlag>(projEnt);
            return;
        }

        health.hp -= proj.damage;
        health.invincibilityFrames = Config::INVINCIBILITY_FRAMES / 2;

        if (reg.all_of<PlayerComponent>(targetEnt)) {
            auto& player = reg.get<PlayerComponent>(targetEnt);
            player.state = PlayerState::Hurt;
            if (health.hp <= 0) {
                health.isDead = true;
                events.publish(PlayerDiedEvent{0});
            } else {
                events.publish(PlayerHurtEvent{health.hp});
            }
        }

        if (reg.all_of<EnemyComponent>(targetEnt) && health.hp <= 0) {
            health.isDead = true;
            reg.get<EnemyComponent>(targetEnt).state = EnemyState::Dead;
            events.publish(EnemyKilledEvent{"enemy",
                reg.get<TransformComponent>(targetEnt).position});
            reg.emplace_or_replace<DestroyFlag>(targetEnt);
        }
    }

    reg.emplace_or_replace<DestroyFlag>(projEnt);
}

} // anonymous namespace

// =============================================================================
// Public
// =============================================================================

void CollisionSystem::update(entt::registry& reg, float /*dt*/, EventBus& events) {
    // 1. Clear player contact flags
    auto playerView = reg.view<PlayerComponent>();
    for (auto e : playerView) {
        auto& p = playerView.get<PlayerComponent>(e);
        p.isGrounded          = false;
        p.isTouchingWallLeft  = false;
        p.isTouchingWallRight = false;
    }

    // 2. Resolve dynamic-vs-static collisions (tiles, walls)
    resolveStaticCollisions(reg);

    // 3. Build spatial grid with dynamic entities
    rebuildGrid(reg);

    // 4. Resolve dynamic-vs-dynamic pairs
    m_pairs.clear();
    resolveDynamicPairs(reg, events);

    // 5. Update player ground/wall probes
    updatePlayerProbes(reg);

    // 6. Destroy flagged entities
    auto destroyView = reg.view<DestroyFlag>();
    reg.destroy(destroyView.begin(), destroyView.end());
}

// =============================================================================
// Spatial grid
// =============================================================================

void CollisionSystem::rebuildGrid(entt::registry& reg) {
    for (auto& [key, vec] : m_grid) {
        vec.clear();
    }

    auto view = reg.view<TransformComponent, ColliderComponent>();
    for (auto entity : view) {
        const auto& transform = view.get<TransformComponent>(entity);
        const auto& collider  = view.get<ColliderComponent>(entity);

        if (collider.isStatic) continue;

        Rect aabb = collider.toRect(transform.position);

        int x0 = static_cast<int>(std::floor(aabb.left()   / CELL_SIZE));
        int y0 = static_cast<int>(std::floor(aabb.top()    / CELL_SIZE));
        int x1 = static_cast<int>(std::floor(aabb.right()  / CELL_SIZE));
        int y1 = static_cast<int>(std::floor(aabb.bottom() / CELL_SIZE));

        for (int cx = x0; cx <= x1; ++cx) {
            for (int cy = y0; cy <= y1; ++cy) {
                m_grid[cellKey(cx, cy)].push_back(entity);
            }
        }
    }
}

// =============================================================================
// Static collision resolution
// =============================================================================

void CollisionSystem::resolveStaticCollisions(entt::registry& reg) {
    auto staticView = reg.view<TransformComponent, ColliderComponent>();
    auto dynView    = reg.view<TransformComponent, ColliderComponent, VelocityComponent>();

    for (auto dynEntity : dynView) {
        auto& dynTransform = dynView.get<TransformComponent>(dynEntity);
        auto& dynCollider  = dynView.get<ColliderComponent>(dynEntity);
        auto& dynVel       = dynView.get<VelocityComponent>(dynEntity);

        if (dynCollider.isStatic || dynCollider.isTrigger) continue;

        Rect dynRect = dynCollider.toRect(dynTransform.position);

        for (auto statEntity : staticView) {
            const auto& statCollider = staticView.get<ColliderComponent>(statEntity);
            if (!statCollider.isStatic || statCollider.isTrigger) continue;

            if (reg.all_of<DestructibleComponent>(statEntity)) {
                if (reg.get<DestructibleComponent>(statEntity).destroyed) continue;
            }

            const auto& statTransform = staticView.get<TransformComponent>(statEntity);
            Rect statRect = statCollider.toRect(statTransform.position);

            if (!AABB::intersects(dynRect, statRect)) continue;

            Vec2f pen = AABB::penetration(dynRect, statRect);

            dynTransform.position.x += pen.x;
            dynTransform.position.y += pen.y;

            if (pen.x != 0.0f) dynVel.velocity.x = 0.0f;
            if (pen.y != 0.0f) {
                dynVel.velocity.y = 0.0f;
                if (pen.y < 0.0f && reg.all_of<PlayerComponent>(dynEntity)) {
                    reg.get<PlayerComponent>(dynEntity).isGrounded = true;
                }
            }

            dynRect = dynCollider.toRect(dynTransform.position);
        }
    }
}

// =============================================================================
// Dynamic pair resolution
// =============================================================================

void CollisionSystem::resolveDynamicPairs(entt::registry& reg, EventBus& events) {
    std::set<std::pair<uint32_t, uint32_t>> seen;

    for (auto& [key, entities] : m_grid) {
        for (size_t i = 0; i < entities.size(); ++i) {
            for (size_t j = i + 1; j < entities.size(); ++j) {
                auto a = entities[i];
                auto b = entities[j];
                uint32_t ai = static_cast<uint32_t>(a);
                uint32_t bi = static_cast<uint32_t>(b);
                auto pair = (ai < bi) ? std::make_pair(ai, bi) : std::make_pair(bi, ai);

                if (!seen.insert(pair).second) continue;
                if (!reg.valid(a) || !reg.valid(b)) continue;

                const auto& tA = reg.get<TransformComponent>(a);
                const auto& cA = reg.get<ColliderComponent>(a);
                const auto& tB = reg.get<TransformComponent>(b);
                const auto& cB = reg.get<ColliderComponent>(b);

                Rect rA = cA.toRect(tA.position);
                Rect rB = cB.toRect(tB.position);

                if (!AABB::intersects(rA, rB)) continue;

                bool aIsPlayer = reg.all_of<PlayerComponent>(a);
                bool bIsPlayer = reg.all_of<PlayerComponent>(b);
                bool aIsEnemy  = reg.all_of<EnemyComponent>(a);
                bool bIsEnemy  = reg.all_of<EnemyComponent>(b);

                // Player vs Enemy
                if (aIsPlayer && bIsEnemy) {
                    handlePlayerEnemyCollision(reg, events, a, b, rA, rB);
                    continue;
                }
                if (bIsPlayer && aIsEnemy) {
                    handlePlayerEnemyCollision(reg, events, b, a, rB, rA);
                    continue;
                }

                // Player vs Collectible
                if (aIsPlayer && reg.all_of<CollectibleComponent>(b)) {
                    handlePlayerCollectible(reg, events, b);
                    continue;
                }
                if (bIsPlayer && reg.all_of<CollectibleComponent>(a)) {
                    handlePlayerCollectible(reg, events, a);
                    continue;
                }

                // Player vs Goal
                if ((aIsPlayer && reg.all_of<GoalComponent>(b)) ||
                    (bIsPlayer && reg.all_of<GoalComponent>(a))) {
                    auto goalEnt = aIsPlayer ? b : a;
                    auto& goal = reg.get<GoalComponent>(goalEnt);
                    if (!goal.reached) {
                        goal.reached = true;
                        events.publish(LevelCompleteEvent{0, 0.0f});
                    }
                    continue;
                }

                // Projectile vs any target
                bool aIsProj = reg.all_of<ProjectileComponent>(a);
                bool bIsProj = reg.all_of<ProjectileComponent>(b);

                if (aIsProj && (bIsPlayer || bIsEnemy)) {
                    handleProjectileHit(reg, events, a, b);
                    continue;
                }
                if (bIsProj && (aIsPlayer || aIsEnemy)) {
                    handleProjectileHit(reg, events, b, a);
                    continue;
                }
            }
        }
    }
}

// =============================================================================
// Player ground / wall probes
// =============================================================================

void CollisionSystem::updatePlayerProbes(entt::registry& reg) {
    auto playerView = reg.view<PlayerComponent, TransformComponent, ColliderComponent>();

    for (auto playerEnt : playerView) {
        auto& player          = playerView.get<PlayerComponent>(playerEnt);
        const auto& pTransform = playerView.get<TransformComponent>(playerEnt);
        const auto& pCollider  = playerView.get<ColliderComponent>(playerEnt);

        Rect pRect = pCollider.toRect(pTransform.position);

        // Thin probes extending 2px from each face
        Rect groundProbe    = {pRect.x + 1.0f,  pRect.bottom(),  pRect.w - 2.0f, 2.0f};
        Rect leftWallProbe  = {pRect.x - 2.0f,  pRect.y + 2.0f,  2.0f, pRect.h - 4.0f};
        Rect rightWallProbe = {pRect.right(),    pRect.y + 2.0f,  2.0f, pRect.h - 4.0f};

        auto staticView = reg.view<TransformComponent, ColliderComponent>();
        for (auto statEnt : staticView) {
            const auto& sc = staticView.get<ColliderComponent>(statEnt);
            if (!sc.isStatic || sc.isTrigger) continue;

            if (reg.all_of<DestructibleComponent>(statEnt)) {
                if (reg.get<DestructibleComponent>(statEnt).destroyed) continue;
            }

            const auto& st = staticView.get<TransformComponent>(statEnt);
            Rect sRect = sc.toRect(st.position);

            if (!player.isGrounded          && AABB::intersects(groundProbe, sRect))    player.isGrounded = true;
            if (!player.isTouchingWallLeft  && AABB::intersects(leftWallProbe, sRect))  player.isTouchingWallLeft = true;
            if (!player.isTouchingWallRight && AABB::intersects(rightWallProbe, sRect)) player.isTouchingWallRight = true;
        }
    }
}
