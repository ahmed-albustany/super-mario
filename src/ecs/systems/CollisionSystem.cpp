#include "ecs/systems/CollisionSystem.hpp"
#include "ecs/Components.hpp"
#include "core/GameConfig.hpp"
#include "core/EventBus.hpp"
#include "core/Events.hpp"
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

void handlePlayerEnemyCollision(entt::registry& reg, EventBus& events,
                                 entt::entity playerEnt, entt::entity enemyEnt,
                                 const Rect& playerRect, const Rect& enemyRect) {
    auto& player  = reg.get<PlayerComponent>(playerEnt);
    auto& pHealth = reg.get<HealthComponent>(playerEnt);
    auto& pVel    = reg.get<VelocityComponent>(playerEnt);

    // Star invincibility — kill enemy on contact
    if (reg.all_of<PowerUpComponent>(playerEnt)) {
        auto& pu = reg.get<PowerUpComponent>(playerEnt);
        if (pu.type == PowerUpType::StarInvincibility) {
            if (reg.all_of<HealthComponent>(enemyEnt)) {
                auto& eHealth = reg.get<HealthComponent>(enemyEnt);
                eHealth.hp = 0;
                eHealth.isDead = true;
            }
            if (reg.all_of<EnemyComponent>(enemyEnt)) {
                reg.get<EnemyComponent>(enemyEnt).state = EnemyState::Dead;
            }
            events.publish(EnemyKilledEvent{"enemy",
                reg.get<TransformComponent>(enemyEnt).position, Config::FIREBALL_KILL_VALUE});
            reg.emplace_or_replace<DestroyFlag>(enemyEnt);
            return;
        }
    }

    // i-frames active — ignore contact
    if (pHealth.invincibilityFrames > 0) return;

    auto& ec = reg.get<EnemyComponent>(enemyEnt);

    // Piranha Plant — can't be stomped, always hurts on contact
    if (ec.type == EnemyType::PiranhaPlant) {
        // Take damage
        goto take_damage;
    }

    // Koopa shell — special handling
    if (ec.state == EnemyState::Shell) {
        // Kick the shell
        float kickDir = (playerRect.center().x < enemyRect.center().x) ? 1.0f : -1.0f;
        ec.shellMoving = !ec.shellMoving;
        ec.facing = (kickDir > 0.0f) ? 1 : -1;
        pVel.velocity.y = Config::PLAYER_STOMP_BOUNCE;
        return;
    }

    // Stomp detection: player's bottom is above enemy's top 40%
    {
        float playerBottom = playerRect.bottom();
        float enemyTopZone = enemyRect.y + enemyRect.h * 0.4f;

        if (playerBottom <= enemyTopZone && pVel.velocity.y > 0.0f) {
            // Stomp!
            pVel.velocity.y = Config::PLAYER_STOMP_BOUNCE;
            Vec2f stompPos = reg.get<TransformComponent>(enemyEnt).position;
            EntityFactory::createParticleEffect(reg, stompPos,
                ParticleEmitterComponent::Effect::StompPoof);

            if (ec.type == EnemyType::Koopa) {
                // Koopa → become shell
                ec.state = EnemyState::Shell;
                ec.isShell = true;
                ec.shellMoving = false;
                EntityFactory::createFloatingText(reg,
                    {stompPos.x, stompPos.y - 16.0f},
                    "+" + std::to_string(Config::ENEMY_STOMP_VALUE),
                    Color{255, 255, 255, 255});
                events.publish(EnemyKilledEvent{"koopa",
                    stompPos, Config::ENEMY_STOMP_VALUE});
            } else if (ec.type == EnemyType::Bowser) {
                // Bowser — takes damage but doesn't die from one stomp
                auto& eHealth = reg.get<HealthComponent>(enemyEnt);
                eHealth.hp -= 1;
                eHealth.invincibilityFrames = 60;
                if (eHealth.hp <= 0) {
                    eHealth.isDead = true;
                    ec.state = EnemyState::Dead;
                    EntityFactory::createFloatingText(reg,
                        {stompPos.x, stompPos.y - 16.0f}, "+5000",
                        Color{255, 220, 50, 255});
                    events.publish(EnemyKilledEvent{"bowser",
                        stompPos, 5000});
                    reg.emplace_or_replace<DestroyFlag>(enemyEnt);
                }
            } else {
                // Goomba — instant kill
                if (reg.all_of<HealthComponent>(enemyEnt)) {
                    auto& eH = reg.get<HealthComponent>(enemyEnt);
                    eH.hp = 0;
                    eH.isDead = true;
                }
                ec.state = EnemyState::Dead;
                EntityFactory::createFloatingText(reg,
                    {stompPos.x, stompPos.y - 16.0f},
                    "+" + std::to_string(Config::ENEMY_STOMP_VALUE),
                    Color{255, 255, 255, 255});
                events.publish(EnemyKilledEvent{"goomba",
                    stompPos, Config::ENEMY_STOMP_VALUE});
                reg.emplace_or_replace<DestroyFlag>(enemyEnt);
            }
            return;
        }
    }

take_damage:
    // Side/below contact — player takes damage based on power state
    if (player.power == MarioPowerState::Small) {
        // Small Mario — die
        pHealth.isDead = true;
        player.state = PlayerState::Dead;
        events.publish(PlayerDiedEvent{0, player.playerIndex});
    } else {
        // Big/Fire Mario — shrink to Small
        player.power = MarioPowerState::Small;
        player.state = PlayerState::Shrinking;
        player.growTimer = PlayerComponent::GROW_DURATION;
        pHealth.invincibilityFrames = Config::INVINCIBILITY_FRAMES;

        // Shrink collider
        auto& coll = reg.get<ColliderComponent>(playerEnt);
        coll.size = {14.0f, 16.0f};

        events.publish(PlayerHurtEvent{1, player.playerIndex});
    }

    float knockDir = (playerRect.center().x < enemyRect.center().x) ? -1.0f : 1.0f;
    pVel.velocity.x = knockDir * 150.0f;
    pVel.velocity.y = -200.0f;
}

void handlePlayerCollectible(entt::registry& reg, EventBus& events,
                              entt::entity playerEnt, entt::entity collectEnt) {
    auto& collectible = reg.get<CollectibleComponent>(collectEnt);
    if (collectible.collected) return;

    collectible.collected = true;
    Vec2f pos = reg.get<TransformComponent>(collectEnt).position;
    auto& player = reg.get<PlayerComponent>(playerEnt);

    switch (collectible.type) {
        case CollectibleType::Coin:
            EntityFactory::createParticleEffect(reg, pos,
                ParticleEmitterComponent::Effect::CoinSparkle);
            EntityFactory::createFloatingText(reg,
                {pos.x, pos.y - 16.0f},
                "+" + std::to_string(collectible.value),
                Color{255, 220, 50, 255});
            events.publish(CoinCollectedEvent{collectible.value, pos, player.playerIndex});
            break;

        case CollectibleType::Mushroom:
            if (player.power == MarioPowerState::Small) {
                player.power = MarioPowerState::Big;
                player.state = PlayerState::Growing;
                player.growTimer = PlayerComponent::GROW_DURATION;
                // Grow collider
                auto& coll = reg.get<ColliderComponent>(playerEnt);
                coll.size = {14.0f, 30.0f};
            }
            events.publish(PlayerPowerUpEvent{"mushroom", player.playerIndex});
            break;

        case CollectibleType::FireFlower:
            if (player.power == MarioPowerState::Small) {
                // Small → Big first
                player.power = MarioPowerState::Big;
                player.state = PlayerState::Growing;
                player.growTimer = PlayerComponent::GROW_DURATION;
                auto& coll = reg.get<ColliderComponent>(playerEnt);
                coll.size = {14.0f, 30.0f};
            } else {
                // Big → Fire
                player.power = MarioPowerState::Fire;
            }
            events.publish(PlayerPowerUpEvent{"fire_flower", player.playerIndex});
            break;

        case CollectibleType::Star: {
            PowerUpComponent pu{};
            pu.type = PowerUpType::StarInvincibility;
            pu.durationRemaining = Config::STAR_DURATION;
            reg.emplace_or_replace<PowerUpComponent>(playerEnt, pu);
            events.publish(PowerUpActivatedEvent{"star", Config::STAR_DURATION, player.playerIndex});
            break;
        }

        case CollectibleType::OneUp:
            events.publish(PlayerPowerUpEvent{"1up", player.playerIndex});
            break;
    }

    reg.emplace_or_replace<DestroyFlag>(collectEnt);
}

void handlePlayerQuestionBlock(entt::registry& reg, EventBus& events,
                                entt::entity playerEnt, entt::entity blockEnt,
                                const Rect& playerRect, const Rect& blockRect) {
    // Only trigger when hitting from below
    float playerTop = playerRect.top();
    float blockBottom = blockRect.bottom();

    if (playerTop > blockBottom - 4.0f) return; // Not hitting from below

    auto& qb = reg.get<QuestionBlockComponent>(blockEnt);
    if (qb.isHit) return; // Already used

    qb.isHit = true;
    qb.bumpTimer = Config::BLOCK_BUMP_DURATION;

    // Switch animation to "empty"
    if (reg.all_of<AnimationComponent>(blockEnt)) {
        reg.get<AnimationComponent>(blockEnt).play("empty");
    }

    Vec2f blockPos = reg.get<TransformComponent>(blockEnt).position;
    Vec2f spawnPos = {blockPos.x, blockPos.y - 32.0f};

    auto& player = reg.get<PlayerComponent>(playerEnt);

    // Spawn contents
    switch (qb.contents) {
        case CollectibleType::Coin:
            EntityFactory::createParticleEffect(reg, spawnPos,
                ParticleEmitterComponent::Effect::CoinSparkle);
            EntityFactory::createFloatingText(reg,
                {spawnPos.x, spawnPos.y - 16.0f},
                "+" + std::to_string(Config::COIN_VALUE),
                Color{255, 220, 50, 255});
            events.publish(CoinCollectedEvent{Config::COIN_VALUE, spawnPos, player.playerIndex});
            events.publish(BlockHitEvent{blockPos, "coin", player.playerIndex});
            break;
        case CollectibleType::Mushroom:
            EntityFactory::createMushroom(reg, spawnPos, true);
            events.publish(BlockHitEvent{blockPos, "mushroom", player.playerIndex});
            break;
        case CollectibleType::FireFlower:
            // Spawn mushroom if Small, fire flower if Big/Fire
            if (player.power == MarioPowerState::Small) {
                EntityFactory::createMushroom(reg, spawnPos, true);
            } else {
                EntityFactory::createFireFlower(reg, spawnPos, true);
            }
            events.publish(BlockHitEvent{blockPos, "fire_flower", player.playerIndex});
            break;
        case CollectibleType::Star:
            EntityFactory::createStar(reg, spawnPos, true);
            events.publish(BlockHitEvent{blockPos, "star", player.playerIndex});
            break;
        case CollectibleType::OneUp:
            EntityFactory::createOneUp(reg, spawnPos, true);
            events.publish(BlockHitEvent{blockPos, "1up", player.playerIndex});
            break;
    }
}

void handlePlayerFlagPole(entt::registry& reg, EventBus& events,
                           entt::entity playerEnt, entt::entity flagEnt) {
    auto& player = reg.get<PlayerComponent>(playerEnt);
    auto& vel = reg.get<VelocityComponent>(playerEnt);
    auto& flagPole = reg.get<FlagPoleComponent>(flagEnt);

    if (flagPole.activated) return;
    flagPole.activated = true;

    player.state = PlayerState::FlagPole;
    vel.velocity.x = 0.0f;
    vel.velocity.y = 200.0f; // slide down

    // Calculate grab height for score
    float playerY = reg.get<TransformComponent>(playerEnt).position.y;
    float height = (flagPole.bottomY - playerY) / (flagPole.bottomY - flagPole.topY);
    height = Math::clamp(height, 0.0f, 1.0f);

    events.publish(FlagPoleGrabbedEvent{height, player.playerIndex});
    events.publish(LevelCompleteEvent{0, 0.0f, player.playerIndex});
}

void handlePlayerPipe(entt::registry& reg,
                       entt::entity playerEnt, entt::entity pipeEnt,
                       const InputManager* /*input*/, bool moveDownHeld) {
    auto& pipe = reg.get<PipeComponent>(pipeEnt);
    if (!pipe.isEnterable) return;

    auto& player = reg.get<PlayerComponent>(playerEnt);
    if (player.state == PlayerState::EnteringPipe) return;

    // Player must be standing on top and pressing down
    if (!player.isGrounded || !moveDownHeld) return;

    player.state = PlayerState::EnteringPipe;
    player.pipeTimer = PlayerComponent::PIPE_DURATION;
    player.pipeTarget = pipe.destination;
}

void handleProjectileHit(entt::registry& reg, EventBus& events,
                          entt::entity projEnt, entt::entity targetEnt) {
    const auto& proj = reg.get<ProjectileComponent>(projEnt);

    if (static_cast<uint32_t>(targetEnt) == proj.ownerId) return;

    // Fireball kills enemies
    if (proj.isFireball && reg.all_of<EnemyComponent>(targetEnt)) {
        auto& ec = reg.get<EnemyComponent>(targetEnt);
        if (ec.type != EnemyType::PiranhaPlant || true) { // fireballs can kill piranha
            if (reg.all_of<HealthComponent>(targetEnt)) {
                auto& eH = reg.get<HealthComponent>(targetEnt);
                eH.hp -= proj.damage;
                if (eH.hp <= 0) {
                    eH.isDead = true;
                    ec.state = EnemyState::Dead;
                    events.publish(EnemyKilledEvent{"enemy",
                        reg.get<TransformComponent>(targetEnt).position,
                        Config::FIREBALL_KILL_VALUE});
                    reg.emplace_or_replace<DestroyFlag>(targetEnt);
                }
            }
        }
        reg.emplace_or_replace<DestroyFlag>(projEnt);
        return;
    }

    // Enemy projectile (Bowser fire) hitting player
    if (proj.isBowserFire && reg.all_of<PlayerComponent>(targetEnt)) {
        auto& player = reg.get<PlayerComponent>(targetEnt);
        auto& health = reg.get<HealthComponent>(targetEnt);

        if (health.invincibilityFrames > 0) {
            reg.emplace_or_replace<DestroyFlag>(projEnt);
            return;
        }

        if (player.power == MarioPowerState::Small) {
            health.isDead = true;
            player.state = PlayerState::Dead;
            events.publish(PlayerDiedEvent{0, player.playerIndex});
        } else {
            player.power = MarioPowerState::Small;
            player.state = PlayerState::Shrinking;
            player.growTimer = PlayerComponent::GROW_DURATION;
            health.invincibilityFrames = Config::INVINCIBILITY_FRAMES;
            auto& coll = reg.get<ColliderComponent>(targetEnt);
            coll.size = {14.0f, 16.0f};
            events.publish(PlayerHurtEvent{1, player.playerIndex});
        }
        reg.emplace_or_replace<DestroyFlag>(projEnt);
        return;
    }

    // Generic projectile hit
    if (reg.all_of<HealthComponent>(targetEnt)) {
        auto& health = reg.get<HealthComponent>(targetEnt);
        if (health.invincibilityFrames > 0) {
            reg.emplace_or_replace<DestroyFlag>(projEnt);
            return;
        }

        health.hp -= proj.damage;
        health.invincibilityFrames = Config::INVINCIBILITY_FRAMES / 2;

        if (reg.all_of<EnemyComponent>(targetEnt) && health.hp <= 0) {
            health.isDead = true;
            reg.get<EnemyComponent>(targetEnt).state = EnemyState::Dead;
            events.publish(EnemyKilledEvent{"enemy",
                reg.get<TransformComponent>(targetEnt).position, Config::ENEMY_STOMP_VALUE});
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
                    handlePlayerCollectible(reg, events, a, b);
                    continue;
                }
                if (bIsPlayer && reg.all_of<CollectibleComponent>(a)) {
                    handlePlayerCollectible(reg, events, b, a);
                    continue;
                }

                // Player vs QuestionBlock (hit from below)
                if (aIsPlayer && reg.all_of<QuestionBlockComponent>(b)) {
                    handlePlayerQuestionBlock(reg, events, a, b, rA, rB);
                    continue;
                }
                if (bIsPlayer && reg.all_of<QuestionBlockComponent>(a)) {
                    handlePlayerQuestionBlock(reg, events, b, a, rB, rA);
                    continue;
                }

                // Player vs FlagPole
                if (aIsPlayer && reg.all_of<FlagPoleComponent>(b)) {
                    handlePlayerFlagPole(reg, events, a, b);
                    continue;
                }
                if (bIsPlayer && reg.all_of<FlagPoleComponent>(a)) {
                    handlePlayerFlagPole(reg, events, b, a);
                    continue;
                }

                // Player vs Goal (backward compat)
                if ((aIsPlayer && reg.all_of<GoalComponent>(b) && !reg.all_of<FlagPoleComponent>(b)) ||
                    (bIsPlayer && reg.all_of<GoalComponent>(a) && !reg.all_of<FlagPoleComponent>(a))) {
                    auto goalEnt = aIsPlayer ? b : a;
                    auto& goal = reg.get<GoalComponent>(goalEnt);
                    if (!goal.reached) {
                        goal.reached = true;
                        events.publish(LevelCompleteEvent{0, 0.0f, 0});
                    }
                    continue;
                }

                // Player vs Pipe
                if (aIsPlayer && reg.all_of<PipeComponent>(b)) {
                    auto& pl = reg.get<PlayerComponent>(a);
                    handlePlayerPipe(reg, a, b, nullptr, pl.isGrounded);
                    continue;
                }
                if (bIsPlayer && reg.all_of<PipeComponent>(a)) {
                    auto& pl = reg.get<PlayerComponent>(b);
                    handlePlayerPipe(reg, b, a, nullptr, pl.isGrounded);
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

                // Koopa shell hitting enemies
                if (aIsEnemy && bIsEnemy) {
                    auto& ecA = reg.get<EnemyComponent>(a);
                    auto& ecB = reg.get<EnemyComponent>(b);
                    if (ecA.state == EnemyState::Shell && ecA.shellMoving) {
                        auto& hB = reg.get<HealthComponent>(b);
                        hB.hp = 0; hB.isDead = true;
                        ecB.state = EnemyState::Dead;
                        reg.emplace_or_replace<DestroyFlag>(b);
                        events.publish(EnemyKilledEvent{"enemy",
                            reg.get<TransformComponent>(b).position, Config::ENEMY_STOMP_VALUE});
                    } else if (ecB.state == EnemyState::Shell && ecB.shellMoving) {
                        auto& hA = reg.get<HealthComponent>(a);
                        hA.hp = 0; hA.isDead = true;
                        ecA.state = EnemyState::Dead;
                        reg.emplace_or_replace<DestroyFlag>(a);
                        events.publish(EnemyKilledEvent{"enemy",
                            reg.get<TransformComponent>(a).position, Config::ENEMY_STOMP_VALUE});
                    }
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
