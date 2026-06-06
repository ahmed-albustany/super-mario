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
#include <string>

class Game;

/// @brief Core gameplay scene — owns the ECS registry, all systems, tilemap, camera.
///        Supports 1P, 2P alternating, and 2P co-op modes.
class GameScene final : public IScene {
public:
    explicit GameScene(Game& game);

    /// @brief Configure player mode before onEnter(). Call from MenuScene.
    /// @param numPlayers 1 or 2
    /// @param coop       true = co-op (simultaneous), false = alternating (if 2P)
    void setPlayerMode(int numPlayers, bool coop);

    void onEnter() override;
    void onExit() override;
    void handleInput(const InputManager& input) override;
    void update(float dt) override;
    void render(IPlatform& platform) override;
    [[nodiscard]] std::string name() const override { return "GameScene"; }

private:
    void loadLevel(const std::string& levelPath);
    void spawnEntities();
    void spawnCurrentPlayer();
    void onPlayerDied(const PlayerDiedEvent& event);
    void onPlayerHurt(const PlayerHurtEvent& event);
    void onCoinCollected(const CoinCollectedEvent& event);
    void onEnemyKilled(const EnemyKilledEvent& event);
    void onLevelComplete(const LevelCompleteEvent& event);
    void onBlockHit(const BlockHitEvent& event);
    void onFlagPoleGrabbed(const FlagPoleGrabbedEvent& event);
    void onPlayerPowerUp(const PlayerPowerUpEvent& event);

    /// @brief Alternating mode: switch to the other player's turn.
    void switchTurn();

    /// @brief Load the next level, or show VictoryScene if all levels completed.
    void advanceLevel();

    Game& m_game;

    // ECS
    entt::registry m_registry;

    // Players (P2 only used in co-op mode)
    Player m_player1;
    Player m_player2;

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

    // Gameplay state (shared with HUDScene)
    GameStatePtr m_state = std::make_shared<GameState>();
    Vec2f m_spawnPoint;

    // Event subscriber IDs
    SubscriberID m_subPlayerDied    = 0;
    SubscriberID m_subPlayerHurt   = 0;
    SubscriberID m_subCoin         = 0;
    SubscriberID m_subEnemyKilled  = 0;
    SubscriberID m_subLevelComplete = 0;
    SubscriberID m_subBlockHit     = 0;
    SubscriberID m_subFlagPole     = 0;
    SubscriberID m_subPowerUp     = 0;
};
