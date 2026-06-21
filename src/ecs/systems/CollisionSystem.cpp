#include "ecs/systems/CollisionSystem.hpp"
#include "ecs/Components.hpp"
#include "core/GameConfig.hpp"
#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "core/InputManager.hpp"
#include "entities/EntityFactory.hpp"
#include "physics/AABB.hpp"
#include "utils/Logger.hpp"

#include <algorithm>
#include <cmath>
#include <set>

// =============================================================================
// Anonymous-namespace helpers
// =============================================================================

namespace {

/// @brief Handle player touching a trap — instant death for most traps.
void handlePlayerTrapCollision(entt::registry& reg, EventBus& events,
                                entt::entity playerEnt, entt::entity trapEnt) {
    auto& player = reg.get<PlayerComponent>(playerEnt);
    auto& health = reg.get<HealthComponent>(playerEnt);
    auto& trap   = reg.get<TrapComponent>(trapEnt);

    if (health.isDead || health.invincibilityFrames > 0) return;

    switch (trap.trapType) {
        case TrapType::Trampoline: {
            // Bounce player upward (3x normal jump)
            auto& vel = reg.get<VelocityComponent>(playerEnt);
            vel.velocity.y = Config::TRAMPOLINE_FORCE;
            player.jumpCount = 1; // Allow double jump after trampoline
            return;
        }

        case TrapType::Fan: {
            // Apply upward force
            if (reg.all_of<VelocityComponent>(playerEnt)) {
                auto& vel = reg.get<VelocityComponent>(playerEnt);
                vel.velocity.y -= trap.fanStrength * 0.016f; // per-frame force
            }
            return;
        }

        case TrapType::MovingPlatform: {
            // Carry player (handled by static collision resolution)
            return;
        }

        case TrapType::FallingPlatform: {
            // Mark that player is on top (triggers shake+fall)
            trap.playerOnTop = true;
            return;
        }

        case TrapType::Fire: {
            if (!trap.isActive) return; // Fire is off, safe to touch
            break; // Fall through to death
        }

        case TrapType::Blocks: {
            if (!trap.isActive) return; // Blocks are off, passthrough
            break; // Fall through to death
        }

        default:
            break;
    }

    // All other traps = instant death
    health.isDead = true;
    player.state = PlayerState::Dead;
    {
        auto trapPos = reg.get<TransformComponent>(trapEnt).position;
        auto playerPos = reg.get<TransformComponent>(playerEnt).position;
        LOG_INFO("CollisionSystem: TRAP KILL p" << player.playerIndex
                 << " trap=(" << trapPos.x << "," << trapPos.y << ")"
                 << " player=(" << playerPos.x << "," << playerPos.y << ")"
                 << " trapType=" << static_cast<int>(trap.trapType));
        events.publish(TrapDeathEvent{"trap", trapPos, player.playerIndex});
    }
    events.publish(PlayerDiedEvent{0, player.playerIndex});
}

/// @brief Handle player collecting a fruit.
void handlePlayerFruit(entt::registry& reg, EventBus& events,
                        entt::entity playerEnt, entt::entity fruitEnt) {
    auto& fruit = reg.get<FruitComponent>(fruitEnt);
    if (fruit.collected) return;

    fruit.collected = true;
    Vec2f pos = reg.get<TransformComponent>(fruitEnt).position;
    auto& player = reg.get<PlayerComponent>(playerEnt);

    // Spawn collection effect
    EntityFactory::createParticleEffect(reg, pos,
        ParticleEmitterComponent::Effect::FruitCollect);
    EntityFactory::createFloatingText(reg,
        {pos.x, pos.y - 16.0f},
        "+" + std::to_string(fruit.value),
        Color{255, 255, 100, 255});

    events.publish(FruitCollectedEvent{
        "fruit", fruit.value, pos, player.playerIndex});

    reg.emplace_or_replace<DestroyFlag>(fruitEnt);
}

/// @brief Handle player hitting a box from below.
void handlePlayerBox(entt::registry& reg, EventBus& events,
                      entt::entity playerEnt, entt::entity boxEnt,
                      const Rect& playerRect, const Rect& boxRect) {
    // Only trigger when hitting from below
    float playerTop = playerRect.top();
    float boxBottom = boxRect.bottom();

    if (playerTop > boxBottom - 4.0f) return;

    auto& box = reg.get<BoxComponent>(boxEnt);
    if (box.isBroken) return;

    box.hitsRemaining--;
    box.bumpTimer = Config::BLOCK_BUMP_DURATION;
    box.isHit = true;

    Vec2f boxPos = reg.get<TransformComponent>(boxEnt).position;
    auto& player = reg.get<PlayerComponent>(playerEnt);

    events.publish(BoxHitEvent{boxPos, box.hitsRemaining, player.playerIndex});

    if (box.hitsRemaining <= 0) {
        box.isBroken = true;
        // Switch to break animation
        if (reg.all_of<AnimationComponent>(boxEnt)) {
            reg.get<AnimationComponent>(boxEnt).play("break");
        }

        // Spawn a random fruit above the box
        Vec2f spawnPos = {boxPos.x, boxPos.y - 32.0f};

        // Pick a random fruit type based on position hash
        int fruitIdx = static_cast<int>(boxPos.x + boxPos.y) % 8;
        FruitType types[] = {
            FruitType::Cherry, FruitType::Apple, FruitType::Orange,
            FruitType::Pineapple, FruitType::Melon, FruitType::Strawberry,
            FruitType::Kiwi, FruitType::Banana
        };
        int values[] = {
            Config::FRUIT_CHERRY_VALUE, Config::FRUIT_APPLE_VALUE,
            Config::FRUIT_ORANGE_VALUE, Config::FRUIT_PINEAPPLE_VALUE,
            Config::FRUIT_MELON_VALUE, Config::FRUIT_STRAWBERRY_VALUE,
            Config::FRUIT_KIWI_VALUE, Config::FRUIT_BANANA_VALUE
        };

        auto fruitEnt = reg.create();
        reg.emplace<TransformComponent>(fruitEnt, TransformComponent{spawnPos});
        reg.emplace<VelocityComponent>(fruitEnt, VelocityComponent{{0.0f, -150.0f}});
        reg.emplace<GravityComponent>(fruitEnt);
        reg.emplace<ColliderComponent>(fruitEnt, ColliderComponent{
            {0.0f, 0.0f}, {16.0f, 16.0f}, true, false
        });
        SpriteComponent fruitSprite;
        fruitSprite.srcRect = {0.0f, 0.0f, 32.0f, 32.0f};
        fruitSprite.zOrder = 4;
        reg.emplace<SpriteComponent>(fruitEnt, fruitSprite);
        reg.emplace<FruitComponent>(fruitEnt, FruitComponent{
            types[fruitIdx], values[fruitIdx], false
        });
        reg.emplace<TagComponent>(fruitEnt, TagComponent{"fruit"});

        events.publish(BoxBreakEvent{boxPos, "fruit", player.playerIndex});
    } else {
        // Switch to hit animation briefly
        if (reg.all_of<AnimationComponent>(boxEnt)) {
            reg.get<AnimationComponent>(boxEnt).play("hit");
        }
    }
}

/// @brief Handle player reaching the trophy (end goal).
void handlePlayerGoal(entt::registry& reg, EventBus& events,
                       entt::entity playerEnt, entt::entity goalEnt) {
    auto& goal = reg.get<GoalComponent>(goalEnt);
    if (goal.reached) return;
    goal.reached = true;

    auto& player = reg.get<PlayerComponent>(playerEnt);
    events.publish(LevelCompleteEvent{0, 0.0f, player.playerIndex});
}

/// @brief Handle player activating a checkpoint.
void handlePlayerCheckpoint(entt::registry& reg, EventBus& events,
                              entt::entity playerEnt, entt::entity cpEnt) {
    auto& checkpoint = reg.get<CheckpointComponent>(cpEnt);
    if (checkpoint.activated) return;
    checkpoint.activated = true;

    auto& player = reg.get<PlayerComponent>(playerEnt);
    Vec2f cpPos = reg.get<TransformComponent>(cpEnt).position;

    // Update animation to flag out
    if (reg.all_of<AnimationComponent>(cpEnt)) {
        reg.get<AnimationComponent>(cpEnt).play("flag_out");
    }

    events.publish(CheckpointActivatedEvent{cpPos, player.playerIndex});
}

// Legacy handlers kept for backward compatibility
void handlePlayerEnemyCollision(entt::registry& reg, EventBus& events,
                                 entt::entity playerEnt, entt::entity enemyEnt,
                                 const Rect& playerRect, const Rect& enemyRect) {
    auto& player  = reg.get<PlayerComponent>(playerEnt);
    auto& pHealth = reg.get<HealthComponent>(playerEnt);
    auto& pVel    = reg.get<VelocityComponent>(playerEnt);

    if (pHealth.invincibilityFrames > 0) return;
    if (pHealth.isDead) return;

    auto& ec = reg.get<EnemyComponent>(enemyEnt);

    // Stomp detection
    float playerBottom = playerRect.bottom();
    float enemyTopZone = enemyRect.y + enemyRect.h * 0.4f;

    if (playerBottom <= enemyTopZone && pVel.velocity.y > 0.0f) {
        pVel.velocity.y = Config::PLAYER_STOMP_BOUNCE;
        if (reg.all_of<HealthComponent>(enemyEnt)) {
            auto& eH = reg.get<HealthComponent>(enemyEnt);
            eH.hp = 0;
            eH.isDead = true;
        }
        ec.state = EnemyState::Dead;
        reg.emplace_or_replace<DestroyFlag>(enemyEnt);
        events.publish(EnemyKilledEvent{"enemy",
            reg.get<TransformComponent>(enemyEnt).position, Config::ENEMY_STOMP_VALUE});
        return;
    }

    // Player takes damage — in PIXEL RUSH, all hits are instant death
    pHealth.isDead = true;
    player.state = PlayerState::Dead;
    events.publish(PlayerDiedEvent{0, player.playerIndex});
}

void handlePlayerCollectible(entt::registry& reg, EventBus& events,
                              entt::entity playerEnt, entt::entity collectEnt) {
    auto& collectible = reg.get<CollectibleComponent>(collectEnt);
    if (collectible.collected) return;

    collectible.collected = true;
    Vec2f pos = reg.get<TransformComponent>(collectEnt).position;
    auto& player = reg.get<PlayerComponent>(playerEnt);

    EntityFactory::createParticleEffect(reg, pos,
        ParticleEmitterComponent::Effect::CoinSparkle);
    events.publish(CoinCollectedEvent{collectible.value, pos, player.playerIndex});

    reg.emplace_or_replace<DestroyFlag>(collectEnt);
}

void handlePlayerQuestionBlock(entt::registry& reg, EventBus& events,
                                entt::entity playerEnt, entt::entity blockEnt,
                                const Rect& playerRect, const Rect& blockRect) {
    float playerTop = playerRect.top();
    float blockBottom = blockRect.bottom();

    if (playerTop > blockBottom - 4.0f) return;

    auto& qb = reg.get<QuestionBlockComponent>(blockEnt);
    if (qb.isHit) return;
    qb.isHit = true;
    qb.bumpTimer = Config::BLOCK_BUMP_DURATION;

    if (reg.all_of<AnimationComponent>(blockEnt)) {
        reg.get<AnimationComponent>(blockEnt).play("empty");
    }

    Vec2f blockPos = reg.get<TransformComponent>(blockEnt).position;
    auto& player = reg.get<PlayerComponent>(playerEnt);
    events.publish(BlockHitEvent{blockPos, "coin", player.playerIndex});
}

void handlePlayerFlagPole(entt::registry& reg, EventBus& events,
                           entt::entity playerEnt, entt::entity flagEnt) {
    auto& flagPole = reg.get<FlagPoleComponent>(flagEnt);
    if (flagPole.activated) return;
    flagPole.activated = true;

    auto& player = reg.get<PlayerComponent>(playerEnt);
    events.publish(FlagPoleGrabbedEvent{1.0f, player.playerIndex});
    events.publish(LevelCompleteEvent{0, 0.0f, player.playerIndex});
}

void handleProjectileHit(entt::registry& reg, EventBus& events,
                          entt::entity projEnt, entt::entity targetEnt) {
    const auto& proj = reg.get<ProjectileComponent>(projEnt);
    if (static_cast<uint32_t>(targetEnt) == proj.ownerId) return;

    if (reg.all_of<HealthComponent>(targetEnt)) {
        auto& health = reg.get<HealthComponent>(targetEnt);
        health.hp -= proj.damage;
        if (health.hp <= 0) {
            health.isDead = true;
            if (reg.all_of<EnemyComponent>(targetEnt)) {
                reg.get<EnemyComponent>(targetEnt).state = EnemyState::Dead;
                events.publish(EnemyKilledEvent{"enemy",
                    reg.get<TransformComponent>(targetEnt).position, Config::ENEMY_STOMP_VALUE});
            }
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
        p.isGrounded = false;
    }

    // Also reset falling platform playerOnTop flags
    auto trapView = reg.view<TrapComponent>();
    for (auto e : trapView) {
        auto& t = trapView.get<TrapComponent>(e);
        if (t.trapType == TrapType::FallingPlatform) {
            t.playerOnTop = false;
        }
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
    m_grid.clear();

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
                if (reg.all_of<DestroyFlag>(a) || reg.all_of<DestroyFlag>(b)) continue;

                const auto& tA = reg.get<TransformComponent>(a);
                const auto& cA = reg.get<ColliderComponent>(a);
                const auto& tB = reg.get<TransformComponent>(b);
                const auto& cB = reg.get<ColliderComponent>(b);

                Rect rA = cA.toRect(tA.position);
                Rect rB = cB.toRect(tB.position);

                if (!AABB::intersects(rA, rB)) continue;

                bool aIsPlayer = reg.all_of<PlayerComponent>(a);
                bool bIsPlayer = reg.all_of<PlayerComponent>(b);

                // Player vs Trap
                if (aIsPlayer && reg.all_of<TrapComponent>(b)) {
                    handlePlayerTrapCollision(reg, events, a, b);
                    continue;
                }
                if (bIsPlayer && reg.all_of<TrapComponent>(a)) {
                    handlePlayerTrapCollision(reg, events, b, a);
                    continue;
                }

                // Player vs Fruit
                if (aIsPlayer && reg.all_of<FruitComponent>(b)) {
                    handlePlayerFruit(reg, events, a, b);
                    continue;
                }
                if (bIsPlayer && reg.all_of<FruitComponent>(a)) {
                    handlePlayerFruit(reg, events, b, a);
                    continue;
                }

                // Player vs Box (hit from below)
                if (aIsPlayer && reg.all_of<BoxComponent>(b)) {
                    handlePlayerBox(reg, events, a, b, rA, rB);
                    continue;
                }
                if (bIsPlayer && reg.all_of<BoxComponent>(a)) {
                    handlePlayerBox(reg, events, b, a, rB, rA);
                    continue;
                }

                // Player vs Checkpoint
                if (aIsPlayer && reg.all_of<CheckpointComponent>(b)) {
                    handlePlayerCheckpoint(reg, events, a, b);
                    continue;
                }
                if (bIsPlayer && reg.all_of<CheckpointComponent>(a)) {
                    handlePlayerCheckpoint(reg, events, b, a);
                    continue;
                }

                // Player vs Goal (trophy)
                if (aIsPlayer && reg.all_of<GoalComponent>(b)) {
                    handlePlayerGoal(reg, events, a, b);
                    continue;
                }
                if (bIsPlayer && reg.all_of<GoalComponent>(a)) {
                    handlePlayerGoal(reg, events, b, a);
                    continue;
                }

                // Player vs FlagPole (legacy)
                if (aIsPlayer && reg.all_of<FlagPoleComponent>(b)) {
                    handlePlayerFlagPole(reg, events, a, b);
                    continue;
                }
                if (bIsPlayer && reg.all_of<FlagPoleComponent>(a)) {
                    handlePlayerFlagPole(reg, events, b, a);
                    continue;
                }

                // Player vs Enemy (legacy)
                bool aIsEnemy = reg.all_of<EnemyComponent>(a);
                bool bIsEnemy = reg.all_of<EnemyComponent>(b);
                if (aIsPlayer && bIsEnemy) {
                    handlePlayerEnemyCollision(reg, events, a, b, rA, rB);
                    continue;
                }
                if (bIsPlayer && aIsEnemy) {
                    handlePlayerEnemyCollision(reg, events, b, a, rB, rA);
                    continue;
                }

                // Player vs Collectible (legacy)
                if (aIsPlayer && reg.all_of<CollectibleComponent>(b)) {
                    handlePlayerCollectible(reg, events, a, b);
                    continue;
                }
                if (bIsPlayer && reg.all_of<CollectibleComponent>(a)) {
                    handlePlayerCollectible(reg, events, b, a);
                    continue;
                }

                // Player vs QuestionBlock (legacy)
                if (aIsPlayer && reg.all_of<QuestionBlockComponent>(b)) {
                    handlePlayerQuestionBlock(reg, events, a, b, rA, rB);
                    continue;
                }
                if (bIsPlayer && reg.all_of<QuestionBlockComponent>(a)) {
                    handlePlayerQuestionBlock(reg, events, b, a, rB, rA);
                    continue;
                }

                // Projectile hits
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

        // Thin ground probe extending 2px below player
        Rect groundProbe = {pRect.x + 1.0f, pRect.bottom(), pRect.w - 2.0f, 2.0f};

        auto staticView = reg.view<TransformComponent, ColliderComponent>();
        for (auto statEnt : staticView) {
            const auto& sc = staticView.get<ColliderComponent>(statEnt);
            if (!sc.isStatic || sc.isTrigger) continue;

            if (reg.all_of<DestructibleComponent>(statEnt)) {
                if (reg.get<DestructibleComponent>(statEnt).destroyed) continue;
            }

            const auto& st = staticView.get<TransformComponent>(statEnt);
            Rect sRect = sc.toRect(st.position);

            if (!player.isGrounded && AABB::intersects(groundProbe, sRect)) {
                player.isGrounded = true;
                break;
            }
        }
    }
}
