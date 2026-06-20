#include "ecs/systems/FruitSystem.hpp"
#include "ecs/Components.hpp"
#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "core/GameConfig.hpp"

void FruitSystem::update(entt::registry& reg, float dt, EventBus& /*events*/) {
    // Animate fruit bobbing
    auto view = reg.view<FruitComponent, TransformComponent>();
    for (auto entity : view) {
        auto& fruit = view.get<FruitComponent>(entity);
        if (fruit.collected) {
            reg.emplace_or_replace<DestroyFlag>(entity);
        }
    }

    // Box bump animation
    auto boxView = reg.view<BoxComponent, TransformComponent>();
    for (auto entity : boxView) {
        auto& box = boxView.get<BoxComponent>(entity);
        if (box.bumpTimer > 0.0f) {
            box.bumpTimer -= dt;
            box.bumpOffset = -8.0f * (box.bumpTimer / Config::BLOCK_BUMP_DURATION);
        } else {
            box.bumpOffset = 0.0f;
        }
    }
}
