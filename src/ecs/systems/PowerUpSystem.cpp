#include "ecs/systems/PowerUpSystem.hpp"
#include "ecs/Components.hpp"
#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "core/GameConfig.hpp"
#include "utils/Logger.hpp"

void PowerUpSystem::update(entt::registry& reg, float dt, EventBus& events) {
    std::vector<entt::entity> expired;

    auto view = reg.view<PowerUpComponent>();

    for (auto entity : view) {
        auto& powerUp = view.get<PowerUpComponent>(entity);

        // First-frame activation: publish event
        float fullDuration = Config::STAR_DURATION;
        if (powerUp.durationRemaining >= fullDuration - dt * 2.0f) {
            events.publish(PowerUpActivatedEvent{"star", powerUp.durationRemaining});
        }

        // Countdown
        powerUp.durationRemaining -= dt;

        // Apply ongoing effects — Star keeps invincibility topped up
        if (reg.all_of<HealthComponent>(entity)) {
            auto& health = reg.get<HealthComponent>(entity);
            health.invincibilityFrames = 10; // refresh each frame
        }

        // Expiry
        if (powerUp.durationRemaining <= 0.0f) {
            expired.push_back(entity);

            if (reg.all_of<HealthComponent>(entity)) {
                reg.get<HealthComponent>(entity).invincibilityFrames = 0;
            }

            events.publish(PowerUpExpiredEvent{"star"});
            LOG_DEBUG("Star power expired on entity " << static_cast<uint32_t>(entity));
        }
    }

    // Remove expired power-up components
    for (auto entity : expired) {
        if (reg.valid(entity)) {
            reg.remove<PowerUpComponent>(entity);
        }
    }
}
