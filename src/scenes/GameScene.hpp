#pragma once

#include "scenes/IScene.hpp"
#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "core/GameState.hpp"
#include "entities/Player.hpp"
#include "physics/PhysicsWorld.hpp"
#include "world/TileMap.hpp"
#include "world/Camera.hpp"
#include "world/Parallax.hpp"
#include "world/LevelLoader.hpp"
#include "ecs/Systems.hpp"

#include <entt/entt.hpp>
#include <array>
#include <string>

class Game;

/// @brief Core gameplay scene — owns the ECS registry, all systems, tilemap, camera.
///        Supports Solo, 2P Alt, 2P Co-op, 4P Co-op, 4P VS modes.
class GameScene final : public IScene {
public:
    explicit GameScene(Game& game);

    /// @brief Configure game mode before onEnter(). Call from MenuScene.
    void setGameMode(GameMode mode);

    void onEnter() override;
    void onExit() override;
    void handleInput(const InputManager& input) override;
    void update(float dt) override;
    void render(IPlatform& platform) override;
    [[nodiscard]] std::string name() const override { return "GameScene"; }

private:
    void loadLevel(const std::string& levelPath);
    void spawnEntities();
    void spawnPlayers();
    void respawnPlayer(int playerIndex);
    void onPlayerDied(const PlayerDiedEvent& event);
    void onFruitCollected(const FruitCollectedEvent& event);
    void onBoxHit(const BoxHitEvent& event);
    void onBoxBreak(const BoxBreakEvent& event);
    void onTrapDeath(const TrapDeathEvent& event);
    void onCheckpointActivated(const CheckpointActivatedEvent& event);
    void onLevelComplete(const LevelCompleteEvent& event);

    /// @brief Alternating mode: switch to the other player's turn.
    void switchTurn();

    /// @brief Load the next level, or show VictoryScene if all levels completed.
    void advanceLevel();

    /// @brief Number of players simultaneously on screen.
    int numSimultaneousPlayers() const;

    Game& m_game;

    // ECS
    entt::registry m_registry;

    // Players (up to 4)
    std::array<Player, 4> m_players;

    // Systems
    PlayerSystem    m_playerSystem;
    EnemyAISystem   m_enemyAISystem;
    PhysicsSystem   m_physicsSystem;
    MovementSystem  m_movementSystem;
    CollisionSystem m_collisionSystem;
    PowerUpSystem   m_powerUpSystem;
    AnimationSystem m_animationSystem;
    RenderSystem    m_renderSystem;
    TrapSystem      m_trapSystem;
    FruitSystem     m_fruitSystem;

    // World
    PhysicsWorld m_physicsWorld;
    TileMap      m_tileMap;
    Camera       m_camera;
    Parallax     m_parallax;
    LevelData    m_levelData;

    // Gameplay state (shared with HUDScene)
    GameStatePtr m_state = std::make_shared<GameState>();
    Vec2f m_spawnPoint;

    // Event subscriber IDs
    SubscriberID m_subPlayerDied       = 0;
    SubscriberID m_subFruitCollected   = 0;
    SubscriberID m_subBoxHit           = 0;
    SubscriberID m_subBoxBreak         = 0;
    SubscriberID m_subTrapDeath        = 0;
    SubscriberID m_subCheckpoint       = 0;
    SubscriberID m_subLevelComplete    = 0;

    // Legacy compatibility subscriber IDs
    SubscriberID m_subCoin             = 0;
    SubscriberID m_subEnemyKilled      = 0;
    SubscriberID m_subBlockHit         = 0;
    SubscriberID m_subFlagPole         = 0;
    SubscriberID m_subPowerUp          = 0;
    SubscriberID m_subPlayerHurt       = 0;
};
