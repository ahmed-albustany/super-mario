#pragma once

#include <string>
#include "utils/Math.hpp"

/// @brief All game event types used with EventBus.

struct PlayerDiedEvent {
    int livesRemaining = 0;
};

struct PlayerHurtEvent {
    int newHP = 0;
};

struct CoinCollectedEvent {
    int value = 0;
    Vec2f position;
};

struct GemCollectedEvent {
    int value = 0;
    Vec2f position;
};

struct EnemyKilledEvent {
    std::string type;
    Vec2f position;
};

struct LevelCompleteEvent {
    int score = 0;
    float timeRemaining = 0.0f;
};

struct GameOverEvent {};

struct PowerUpActivatedEvent {
    std::string type;
    float duration = 0.0f;
};

