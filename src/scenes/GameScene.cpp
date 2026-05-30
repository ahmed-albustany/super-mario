#include "scenes/GameScene.hpp"
#include "scenes/PauseScene.hpp"
#include "scenes/GameOverScene.hpp"
#include "scenes/HUDScene.hpp"
#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/InputManager.hpp"
#include "core/ResourceManager.hpp"
#include "entities/EntityFactory.hpp"
#include "ecs/Components.hpp"
#include "utils/Logger.hpp"
#include "utils/Math.hpp"

GameScene::GameScene(Game& game) : m_game(game) {}

// =============================================================================
// Lifecycle
// =============================================================================

void GameScene::onEnter() {
    LOG_INFO("GameScene: entering");

    // Reset gameplay state
    m_score     = 0;
    m_lives     = Config::DEFAULT_LIVES;
    m_coins     = 0;
    m_gems      = 0;
    m_levelWon  = false;
    m_gameOver  = false;

    // Subscribe to events
    auto& bus = m_game.events();
    m_subPlayerDied    = bus.subscribe<PlayerDiedEvent>(
        [this](const PlayerDiedEvent& e) { onPlayerDied(e); });
    m_subPlayerHurt    = bus.subscribe<PlayerHurtEvent>(
        [this](const PlayerHurtEvent& e) { onPlayerHurt(e); });
    m_subCoin          = bus.subscribe<CoinCollectedEvent>(
        [this](const CoinCollectedEvent& e) { onCoinCollected(e); });
    m_subGem           = bus.subscribe<GemCollectedEvent>(
        [this](const GemCollectedEvent& e) { onGemCollected(e); });
    m_subEnemyKilled   = bus.subscribe<EnemyKilledEvent>(
        [this](const EnemyKilledEvent& e) { onEnemyKilled(e); });
    m_subLevelComplete = bus.subscribe<LevelCompleteEvent>(
        [this](const LevelCompleteEvent& e) { onLevelComplete(e); });

    // Load level
    loadLevel("levels/level_01.json");

    // Push HUD as overlay on top of us
    m_game.scenes().push(std::make_unique<HUDScene>(m_game, m_score, m_lives,
                                                      m_coins, m_gems, m_levelTimer));
}

void GameScene::onExit() {
    // Unsubscribe from events
    auto& bus = m_game.events();
    bus.unsubscribe<PlayerDiedEvent>(m_subPlayerDied);
    bus.unsubscribe<PlayerHurtEvent>(m_subPlayerHurt);
    bus.unsubscribe<CoinCollectedEvent>(m_subCoin);
    bus.unsubscribe<GemCollectedEvent>(m_subGem);
    bus.unsubscribe<EnemyKilledEvent>(m_subEnemyKilled);
    bus.unsubscribe<LevelCompleteEvent>(m_subLevelComplete);

    // Clear ECS
    m_registry.clear();

    LOG_INFO("GameScene: exited");
}

// =============================================================================
// Level loading
// =============================================================================

void GameScene::loadLevel(const std::string& levelPath) {
    auto levelOpt = LevelLoader::load(levelPath);
    if (!levelOpt) {
        LOG_ERROR("GameScene: failed to load level");
        return;
    }

    m_levelData = std::move(*levelOpt);
    m_levelTimer = m_levelData.timeLimit;

    // Physics
    m_physicsWorld.reset();
    m_physicsWorld.loadFromLevel(m_levelData.gravity);

    // Tilemap
    auto tileset = ResourceManager::instance().getTexture("tileset_ruins");
    TextureHandle tilesetHandle = tileset.value_or(TextureHandle{0});
    m_tileMap.load(m_levelData.tiles, m_levelData.widthTiles, m_levelData.heightTiles,
                   tilesetHandle, 16, m_registry);

    // Camera
    m_camera.setBounds(m_tileMap.getPixelWidth(), m_tileMap.getPixelHeight());

    // Parallax backgrounds
    m_parallax.clear();
    auto& rm = ResourceManager::instance();
    auto bgFar  = rm.getTexture("bg_ruins_far");
    auto bgMid  = rm.getTexture("bg_ruins_mid");
    auto bgNear = rm.getTexture("bg_ruins_near");
    if (bgFar && bgMid && bgNear) {
        m_parallax.addDefaultLayers(
            *bgFar,  1280.0f, 720.0f,
            *bgMid,  1280.0f, 720.0f,
            *bgNear, 1280.0f, 720.0f
        );
    }

    // Data-driven entity spawning from level file
    m_spawnPoint = m_levelData.playerSpawn;
    spawnEntities();

    LOG_INFO("GameScene: level '" << m_levelData.name << "' loaded");
}

void GameScene::spawnEntities() {
    // ---- Player ----
    m_player.spawn(m_registry, m_spawnPoint);

    // ---- Enemies (data-driven from LevelData) ----
    for (const auto& esd : m_levelData.enemies) {
        Vec2f pos{esd.x, esd.y};

        if (esd.type == "jumper") {
            (void)EntityFactory::createJumper(m_registry, pos, esd.patrolLeft, esd.patrolRight);
        } else if (esd.type == "shooter") {
            (void)EntityFactory::createShooter(m_registry, pos, esd.facing == -1);
        } else if (esd.type == "guardian") {
            (void)EntityFactory::createGuardian(m_registry, pos, esd.patrolLeft, esd.patrolRight);
        } else {
            // Default: walker
            (void)EntityFactory::createWalker(m_registry, pos, esd.patrolLeft, esd.patrolRight);
        }
    }

    // ---- Collectibles (data-driven from LevelData) ----
    for (const auto& csd : m_levelData.collectibles) {
        Vec2f pos{csd.x, csd.y};

        if (csd.type == "gem_shard") {
            (void)EntityFactory::createGemShard(m_registry, pos);
        } else if (csd.type == "power_crystal") {
            (void)EntityFactory::createPowerCrystal(m_registry, pos);
        } else {
            (void)EntityFactory::createCoin(m_registry, pos);
        }
    }

    // ---- Goal ----
    (void)EntityFactory::createGoal(m_registry, m_levelData.goalPosition);
}

// =============================================================================
// Input / Update / Render
// =============================================================================

void GameScene::handleInput(const InputManager& input) {
    if (input.isJustPressed(Action::Pause)) {
        m_game.scenes().push(std::make_unique<PauseScene>(m_game));
    }
}

void GameScene::update(float dt) {
    if (m_gameOver || m_levelWon) return;

    // Countdown level timer
    m_levelTimer -= dt;
    if (m_levelTimer <= 0.0f) {
        m_levelTimer = 0.0f;
        m_gameOver = true;
        m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false, m_score));
        return;
    }

    // ---- System tick order ----
    m_playerSystem.update(m_registry, dt, m_game.input(), m_game.events());
    m_enemyAISystem.update(m_registry, dt);
    m_physicsSystem.update(m_registry, dt);
    m_movementSystem.update(m_registry, dt);
    m_collisionSystem.update(m_registry, dt, m_game.events());
    m_powerUpSystem.update(m_registry, dt, m_game.events());
    m_animationSystem.update(m_registry, dt);

    // Update camera to follow player
    if (m_player.isValid(m_registry)) {
        const auto& pTransform = m_registry.get<TransformComponent>(m_player.getEntity());
        m_camera.setTarget(pTransform.position);
    }
    m_camera.update(dt);

    // Apply camera offset to platform
    m_game.platform().setCameraOffset(m_camera.getViewOffset());

    // Check for player falling into pit
    if (m_player.isValid(m_registry)) {
        const auto& pos = m_registry.get<TransformComponent>(m_player.getEntity()).position;
        if (pos.y > m_tileMap.getPixelHeight() + 100.0f) {
            // Fell off the bottom
            --m_lives;
            if (m_lives <= 0) {
                m_gameOver = true;
                m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false, m_score));
            } else {
                m_player.respawn(m_registry, m_spawnPoint);
                m_camera.setTarget(m_spawnPoint);
            }
        }
    }
}

void GameScene::render(IPlatform& platform) {
    // Parallax background (behind everything)
    m_parallax.render(platform, m_camera.getViewOffset());

    // Tilemap
    m_tileMap.render(platform, m_camera.getViewOffset());

    // Entities via RenderSystem
    m_renderSystem.update(m_registry, platform);
}

// =============================================================================
// Event handlers
// =============================================================================

void GameScene::onPlayerDied(const PlayerDiedEvent& /*event*/) {
    --m_lives;
    if (m_lives <= 0) {
        m_gameOver = true;
        m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false, m_score));
    } else {
        m_player.respawn(m_registry, m_spawnPoint);
        m_camera.setTarget(m_spawnPoint);
    }
}

void GameScene::onPlayerHurt(const PlayerHurtEvent& /*event*/) {
    m_camera.addShake(6.0f);
}

void GameScene::onCoinCollected(const CoinCollectedEvent& event) {
    m_score = Math::safeAdd(m_score, event.value);
    ++m_coins;
}

void GameScene::onGemCollected(const GemCollectedEvent& event) {
    m_score = Math::safeAdd(m_score, event.value);
    ++m_gems;
}

void GameScene::onEnemyKilled(const EnemyKilledEvent& /*event*/) {
    m_score = Math::safeAdd(m_score, Config::ENEMY_STOMP_VALUE);
}

void GameScene::onLevelComplete(const LevelCompleteEvent& /*event*/) {
    // Time bonus
    int timeBonus = static_cast<int>(m_levelTimer) * 10;
    m_score = Math::safeAdd(m_score, timeBonus);
    m_levelWon = true;
    m_game.scenes().push(std::make_unique<GameOverScene>(m_game, true, m_score));
}
