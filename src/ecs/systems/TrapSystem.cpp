#include "ecs/systems/TrapSystem.hpp"
#include "ecs/Components.hpp"
#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "core/GameConfig.hpp"
#include "utils/Math.hpp"

#include <cmath>

void TrapSystem::update(entt::registry& reg, float dt, EventBus& /*events*/) {
    auto view = reg.view<TrapComponent, TransformComponent>();

    for (auto entity : view) {
        auto& trap = view.get<TrapComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);

        switch (trap.trapType) {
            case TrapType::Saw: {
                // Rotate continuously
                transform.rotation += 360.0f * trap.speed * dt;
                if (transform.rotation >= 360.0f) transform.rotation -= 360.0f;
                break;
            }

            case TrapType::SpikeHead: {
                // Idle, then charge toward nearest player
                if (!trap.isActive) {
                    trap.timer += dt;
                    if (trap.timer > 2.0f) {
                        trap.isActive = true;
                        trap.timer = 0.0f;
                    }
                    break;
                }

                // Find nearest player
                float nearestDist = 99999.0f;
                Vec2f targetPos = transform.position;
                auto playerView = reg.view<PlayerComponent, TransformComponent>();
                for (auto pEnt : playerView) {
                    const auto& pPos = playerView.get<TransformComponent>(pEnt).position;
                    float dx = pPos.x - transform.position.x;
                    float dy = pPos.y - transform.position.y;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist < nearestDist) {
                        nearestDist = dist;
                        targetPos = pPos;
                    }
                }

                // Charge toward player
                if (nearestDist < 300.0f) {
                    float dx = targetPos.x - transform.position.x;
                    float dy = targetPos.y - transform.position.y;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist > 1.0f) {
                        float speed = Config::SPIKE_HEAD_CHARGE_SPEED;
                        transform.position.x += (dx / dist) * speed * dt;
                        transform.position.y += (dy / dist) * speed * dt;
                    }
                }
                break;
            }

            case TrapType::RockHead: {
                // Falls from ceiling when player is below
                if (!trap.isFalling) {
                    auto playerView = reg.view<PlayerComponent, TransformComponent>();
                    for (auto pEnt : playerView) {
                        const auto& pPos = playerView.get<TransformComponent>(pEnt).position;
                        float dx = std::abs(pPos.x - transform.position.x);
                        if (dx < 32.0f && pPos.y > transform.position.y) {
                            trap.isFalling = true;
                            break;
                        }
                    }
                } else {
                    // Fall
                    if (reg.all_of<VelocityComponent>(entity)) {
                        reg.get<VelocityComponent>(entity).velocity.y += Config::GRAVITY * dt;
                    }
                }
                break;
            }

            case TrapType::Fire: {
                // Toggle on/off
                trap.timer += dt;
                if (trap.isActive) {
                    if (trap.timer >= trap.onTime) {
                        trap.isActive = false;
                        trap.timer = 0.0f;
                    }
                } else {
                    if (trap.timer >= trap.offTime) {
                        trap.isActive = true;
                        trap.timer = 0.0f;
                    }
                }
                // Update visibility
                if (reg.all_of<SpriteComponent>(entity)) {
                    reg.get<SpriteComponent>(entity).visible = trap.isActive;
                }
                break;
            }

            case TrapType::Arrow: {
                // Shoot horizontally from wall
                trap.timer += dt;
                if (trap.timer >= 3.0f) {
                    trap.timer = 0.0f;
                    // Create arrow projectile
                    float dir = (trap.direction == "left") ? -1.0f : 1.0f;
                    auto arrow = reg.create();
                    reg.emplace<TransformComponent>(arrow, TransformComponent{transform.position});
                    reg.emplace<VelocityComponent>(arrow, VelocityComponent{
                        {dir * 200.0f, 0.0f}
                    });
                    reg.emplace<ColliderComponent>(arrow, ColliderComponent{
                        {0.0f, 0.0f}, {16.0f, 8.0f}, true, false
                    });
                    reg.emplace<TrapComponent>(arrow, TrapComponent{TrapType::Arrow, true, 0.0f, 1.0f});
                    SpriteComponent arrowSprite;
                    arrowSprite.srcRect = {0.0f, 0.0f, 16.0f, 8.0f};
                    arrowSprite.zOrder = 6;
                    arrowSprite.flipX = (dir < 0.0f);
                    reg.emplace<SpriteComponent>(arrow, arrowSprite);
                    reg.emplace<TagComponent>(arrow, TagComponent{"arrow_projectile"});
                    // Auto-destroy after 5 seconds
                    auto& arrowTrap = reg.get<TrapComponent>(arrow);
                    arrowTrap.onTime = 5.0f;
                }
                break;
            }

            case TrapType::FallingPlatform: {
                if (trap.playerOnTop && !trap.isFalling) {
                    trap.shakeTimer += dt;
                    // Shake effect
                    transform.position.x += std::sin(trap.shakeTimer * 40.0f) * 1.0f;
                    if (trap.shakeTimer >= Config::FALLING_PLATFORM_SHAKE_TIME) {
                        trap.isFalling = true;
                        trap.shakeTimer = 0.0f;
                    }
                }
                if (trap.isFalling) {
                    if (reg.all_of<VelocityComponent>(entity)) {
                        reg.get<VelocityComponent>(entity).velocity.y += Config::GRAVITY * 0.5f * dt;
                    }
                }
                break;
            }

            case TrapType::MovingPlatform: {
                if (trap.path.size() < 2) break;
                auto targetIdx = static_cast<size_t>(trap.pathIndex);
                Vec2f target = trap.path[targetIdx];
                // Convert from tile coords to pixels
                Vec2f targetPx = {target.x * 16.0f, target.y * 16.0f};

                float dx = targetPx.x - transform.position.x;
                float dy = targetPx.y - transform.position.y;
                float dist = std::sqrt(dx * dx + dy * dy);

                if (dist < 2.0f) {
                    // Reached waypoint, go to next
                    if (trap.pathForward) {
                        trap.pathIndex++;
                        if (trap.pathIndex >= static_cast<int>(trap.path.size())) {
                            trap.pathIndex = static_cast<int>(trap.path.size()) - 2;
                            trap.pathForward = false;
                        }
                    } else {
                        trap.pathIndex--;
                        if (trap.pathIndex < 0) {
                            trap.pathIndex = 1;
                            trap.pathForward = true;
                        }
                    }
                } else {
                    float moveSpeed = trap.speed;
                    transform.position.x += (dx / dist) * moveSpeed * dt;
                    transform.position.y += (dy / dist) * moveSpeed * dt;
                }
                break;
            }

            case TrapType::Fan: {
                // Fan pushes players upward within its area
                // Effect applied in CollisionSystem when player overlaps fan trigger
                break;
            }

            case TrapType::SpikedBall: {
                // Swing on chain in arc
                trap.swingAngle += trap.speed * dt;
                float radius = trap.chainLength * 16.0f;
                float offsetX = std::sin(trap.swingAngle) * radius;
                float offsetY = std::cos(trap.swingAngle) * radius;
                transform.position.x = trap.anchorPos.x + offsetX;
                transform.position.y = trap.anchorPos.y + offsetY;
                break;
            }

            case TrapType::Spikes:
                // Static — instant death on touch (handled in CollisionSystem)
                break;

            case TrapType::Trampoline:
                // Bounce effect handled in CollisionSystem
                break;

            case TrapType::Blocks: {
                // Toggle blocks on/off
                trap.timer += dt;
                if (trap.timer >= trap.onTime + trap.offTime) {
                    trap.timer = 0.0f;
                }
                trap.isActive = trap.timer < trap.onTime;
                if (reg.all_of<ColliderComponent>(entity)) {
                    // When inactive, blocks become passthrough
                    reg.get<ColliderComponent>(entity).isTrigger = !trap.isActive;
                }
                if (reg.all_of<SpriteComponent>(entity)) {
                    reg.get<SpriteComponent>(entity).visible = trap.isActive;
                }
                break;
            }
        }
    }
}
