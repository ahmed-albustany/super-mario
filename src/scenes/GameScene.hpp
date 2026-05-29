#pragma once

#include "scenes/IScene.hpp"
#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "entities/Player.hpp"
#include "physics/PhysicsWorld.hpp"
#include "world/TileMap.hpp"
#include "world/Camera.hpp"
#include "world/Parallax.hpp"
#include "world/LevelLoader.hpp"
#include "ecs/Systems.hpp"

#include <entt/entt.hpp>
#include <string>

class Game;

/// @brief Core gameplay scene — owns the ECS registry, all systems, tilemap, camera.
///        Tick order: Player → EnemyAI → Physics → Movement → Collision →
///        PowerUp → Animation → Render.
class GameScene final : public IScene {
public:
    explicit GameScene(Game& game);

    void onEnter() override;
    void onExit() override;
    void handleInput(const InputManager& input) override;
    void update(float dt) override;
    void render(IPlatform& platform) override;
    [[nodiscard]] std::string name() const override { return "GameScene"; }

private:
    void loadLevel(const std::string& levelPath);
    void spawnEntities();
    void onPlayerDied(const PlayerDiedEvent& event);
    void onPlayerHurt(const PlayerHurtEvent& event);
    void onCoinCollected(const CoinCollectedEvent& event);
    void onGemCollected(const GemCollectedEvent& event);
    void onEnemyKilled(const EnemyKilledEvent& event);
    void onLevelComplete(const LevelCompleteEvent& event);

    Game& m_game;

    // ECS
    entt::registry m_registry;

    // Player wrapper
    Player m_player;

    // Systems
    PlayerSystem    m_playerSystem;
    EnemyAISystem   m_enemyAISystem;
    PhysicsSystem   m_physicsSystem;
    MovementSystem  m_movementSystem;
    CollisionSystem m_collisionSystem;
    PowerUpSystem   m_powerUpSystem;
    AnimationSystem m_animationSystem;
    RenderSystem    m_renderSystem;

    // World
    PhysicsWorld m_physicsWorld;
    TileMap      m_tileMap;
    Camera       m_camera;
    Parallax     m_parallax;
    LevelData    m_levelData;

    // Gameplay state (RAM only — no persistence)
    int   m_score      = 0;
    int   m_lives      = 3;
    int   m_coins      = 0;
    int   m_gems       = 0;
    float m_levelTimer = 300.0f;
    bool  m_levelWon   = false;
    bool  m_gameOver   = false;
    Vec2f m_spawnPoint;

    // Event subscriber IDs
    SubscriberID m_subPlayerDied   = 0;
    SubscriberID m_subPlayerHurt   = 0;
    SubscriberID m_subCoin         = 0;
    SubscriberID m_subGem          = 0;
    SubscriberID m_subEnemyKilled  = 0;
    SubscriberID m_subLevelComplete = 0;
};
