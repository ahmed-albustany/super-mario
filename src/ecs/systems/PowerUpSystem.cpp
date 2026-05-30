#include "ecs/systems/PowerUpSystem.hpp"
#include "ecs/Components.hpp"
#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "core/GameConfig.hpp"
#include "utils/Logger.hpp"

void PowerUpSystem::update(entt::registry& reg, float dt, EventBus& events) {
    // Collect entities whose power-up has expired (can't remove during iteration)
    std::vector<entt::entity> expired;

    auto view = reg.view<PowerUpComponent>();

    for (auto entity : view) {
        auto& powerUp = view.get<PowerUpComponent>(entity);

        // First-frame activation: publish event
        // Detect first frame by checking if duration is near the full value
        float fullDuration = Config::POWER_CRYSTAL_DURATION;
        if (powerUp.durationRemaining >= fullDuration - dt * 2.0f) {
            std::string typeName;
            switch (powerUp.type) {
                case PowerUpType::Invincibility: typeName = "invincibility"; break;
                case PowerUpType::SpeedBoost:    typeName = "speed_boost";   break;
            }
            events.publish(PowerUpActivatedEvent{typeName, powerUp.durationRemaining});
        }

        // Countdown
        powerUp.durationRemaining -= dt;

        // Apply ongoing effects
        switch (powerUp.type) {
            case PowerUpType::Invincibility:
                // Keep invincibility frames topped up while active
                if (reg.all_of<HealthComponent>(entity)) {
                    auto& health = reg.get<HealthComponent>(entity);
                    health.invincibilityFrames = 10; // refresh each frame
                }
                break;

            case PowerUpType::SpeedBoost:
                // Speed multiplier is read by PlayerSystem when applying horizontal movement.
                // No additional per-frame work needed here.
                break;
        }

        // Expiry
        if (powerUp.durationRemaining <= 0.0f) {
            expired.push_back(entity);

            // Revert effects
            switch (powerUp.type) {
                case PowerUpType::Invincibility:
                    if (reg.all_of<HealthComponent>(entity)) {
                        reg.get<HealthComponent>(entity).invincibilityFrames = 0;
                    }
                    break;
                case PowerUpType::SpeedBoost:
                    // No persistent state to revert — PlayerSystem checks for
                    // PowerUpComponent existence, so removing it is sufficient.
                    break;
            }

            // Notify audio system
            std::string expiredType;
            switch (powerUp.type) {
                case PowerUpType::Invincibility: expiredType = "invincibility"; break;
                case PowerUpType::SpeedBoost:    expiredType = "speed_boost";   break;
            }
            events.publish(PowerUpExpiredEvent{expiredType});

            LOG_DEBUG("Power-up expired on entity " << static_cast<uint32_t>(entity));
        }
    }

    // Remove expired power-up components
    for (auto entity : expired) {
        if (reg.valid(entity)) {
            reg.remove<PowerUpComponent>(entity);
        }
    }
}
