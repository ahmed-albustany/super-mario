#include "scenes/GameScene.hpp"
#include "scenes/PauseScene.hpp"
#include "scenes/GameOverScene.hpp"
#include "scenes/HUDScene.hpp"
#include "scenes/GetReadyScene.hpp"
#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/InputManager.hpp"
#include "core/ResourceManager.hpp"
#include "audio/AudioManager.hpp"
#include "entities/EntityFactory.hpp"
#include "ecs/Components.hpp"
#include "utils/Logger.hpp"
#include "utils/Math.hpp"

GameScene::GameScene(Game& game) : m_game(game) {}

void GameScene::setPlayerMode(int numPlayers, bool coop) {
    m_state->numPlayers    = numPlayers;
    m_state->coopMode      = coop;
    m_state->currentPlayer = 0;
}

// =============================================================================
// Lifecycle
// =============================================================================

void GameScene::onEnter() {
    LOG_INFO("GameScene: entering (" << m_state->numPlayers << "P"
             << (m_state->coopMode ? " co-op" : "") << ")");

    // Reset gameplay state (preserve mode settings)
    int np = m_state->numPlayers;
    bool coop = m_state->coopMode;
    m_state = std::make_shared<GameState>();
    m_state->numPlayers    = np;
    m_state->coopMode      = coop;
    m_state->currentPlayer = 0;
    m_state->p1.lives      = Config::DEFAULT_LIVES;
    m_state->p2.lives      = Config::DEFAULT_LIVES;

    // Subscribe to events
    auto& bus = m_game.events();
    m_subPlayerDied    = bus.subscribe<PlayerDiedEvent>(
        [this](const PlayerDiedEvent& e) { onPlayerDied(e); });
    m_subPlayerHurt    = bus.subscribe<PlayerHurtEvent>(
        [this](const PlayerHurtEvent& e) { onPlayerHurt(e); });
    m_subCoin          = bus.subscribe<CoinCollectedEvent>(
        [this](const CoinCollectedEvent& e) { onCoinCollected(e); });
    m_subEnemyKilled   = bus.subscribe<EnemyKilledEvent>(
        [this](const EnemyKilledEvent& e) { onEnemyKilled(e); });
    m_subLevelComplete = bus.subscribe<LevelCompleteEvent>(
        [this](const LevelCompleteEvent& e) { onLevelComplete(e); });
    m_subBlockHit      = bus.subscribe<BlockHitEvent>(
        [this](const BlockHitEvent& e) { onBlockHit(e); });
    m_subFlagPole      = bus.subscribe<FlagPoleGrabbedEvent>(
        [this](const FlagPoleGrabbedEvent& e) { onFlagPoleGrabbed(e); });

    // Load level
    loadLevel("levels/level_01.json");

    // Push HUD as overlay on top of us
    m_game.scenes().push(std::make_unique<HUDScene>(m_game, m_state));

    // Start gameplay music
    AudioManager::instance().playMusic("overworld_theme", true);
}

void GameScene::onExit() {
    AudioManager::instance().stopMusic();

    // Pop the HUD overlay that we pushed in onEnter
    auto& sm = m_game.scenes();
    if (sm.current() && sm.current()->name() == "HUDScene") {
        sm.pop();
    }

    auto& bus = m_game.events();
    bus.unsubscribe<PlayerDiedEvent>(m_subPlayerDied);
    bus.unsubscribe<PlayerHurtEvent>(m_subPlayerHurt);
    bus.unsubscribe<CoinCollectedEvent>(m_subCoin);
    bus.unsubscribe<EnemyKilledEvent>(m_subEnemyKilled);
    bus.unsubscribe<LevelCompleteEvent>(m_subLevelComplete);
    bus.unsubscribe<BlockHitEvent>(m_subBlockHit);
    bus.unsubscribe<FlagPoleGrabbedEvent>(m_subFlagPole);

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
    m_state->levelTimer = m_levelData.timeLimit;

    // Physics
    m_physicsWorld.reset();
    m_physicsWorld.loadFromLevel(m_levelData.gravity);

    // Tilemap
    auto tileset = ResourceManager::instance().getTexture("tileset_mario");
    TextureHandle tilesetHandle = tileset.value_or(TextureHandle{0});
    m_tileMap.load(m_levelData.tiles, m_levelData.widthTiles, m_levelData.heightTiles,
                   tilesetHandle, 16, m_registry);

    // Camera
    m_camera.setBounds(m_tileMap.getPixelWidth(), m_tileMap.getPixelHeight());

    // Parallax backgrounds
    m_parallax.clear();
    auto& rm = ResourceManager::instance();
    auto bgFar  = rm.getTexture("bg_sky");
    auto bgMid  = rm.getTexture("bg_hills");
    auto bgNear = rm.getTexture("bg_bushes");
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
    // ---- Player(s) ----
    if (m_state->coopMode && m_state->numPlayers == 2) {
        // Co-op: spawn both players simultaneously
        m_player1.spawn(m_registry, m_spawnPoint, 0);
        Vec2f p2Spawn = {m_spawnPoint.x + 24.0f, m_spawnPoint.y};
        m_player2.spawn(m_registry, p2Spawn, 1);
    } else {
        // Single player or alternating: spawn only current player
        spawnCurrentPlayer();
    }

    // ---- Enemies ----
    for (const auto& esd : m_levelData.enemies) {
        Vec2f pos{esd.x, esd.y};

        if (esd.type == "koopa") {
            (void)EntityFactory::createKoopa(m_registry, pos, esd.patrolLeft, esd.patrolRight);
        } else if (esd.type == "piranha_plant") {
            (void)EntityFactory::createPiranhaPlant(m_registry, pos);
        } else if (esd.type == "bowser") {
            (void)EntityFactory::createBowser(m_registry, pos, esd.patrolLeft, esd.patrolRight);
        } else {
            (void)EntityFactory::createGoomba(m_registry, pos, esd.patrolLeft, esd.patrolRight);
        }
    }

    // ---- Collectibles ----
    for (const auto& csd : m_levelData.collectibles) {
        Vec2f pos{csd.x, csd.y};

        if (csd.type == "mushroom") {
            (void)EntityFactory::createMushroom(m_registry, pos);
        } else if (csd.type == "fire_flower") {
            (void)EntityFactory::createFireFlower(m_registry, pos);
        } else if (csd.type == "star") {
            (void)EntityFactory::createStar(m_registry, pos);
        } else if (csd.type == "1up") {
            (void)EntityFactory::createOneUp(m_registry, pos);
        } else {
            (void)EntityFactory::createCoin(m_registry, pos);
        }
    }

    // ---- Question Blocks ----
    for (const auto& qsd : m_levelData.questionBlocks) {
        Vec2f pos{qsd.x, qsd.y};
        CollectibleType contents = CollectibleType::Coin;
        if (qsd.contents == "mushroom")      contents = CollectibleType::Mushroom;
        else if (qsd.contents == "fire_flower") contents = CollectibleType::FireFlower;
        else if (qsd.contents == "star")     contents = CollectibleType::Star;
        else if (qsd.contents == "1up")      contents = CollectibleType::OneUp;

        (void)EntityFactory::createQuestionBlock(m_registry, pos, contents);
    }

    // ---- Pipes ----
    for (const auto& psd : m_levelData.pipes) {
        Vec2f pos{psd.x, psd.y};
        Vec2f dest{psd.destX, psd.destY};
        (void)EntityFactory::createPipe(m_registry, pos, psd.enterable, dest);
    }

    // ---- Goal (Flagpole) ----
    if (m_levelData.goalType == "flagpole") {
        (void)EntityFactory::createFlagPole(m_registry, m_levelData.goalPosition,
                                            m_levelData.flagPoleHeight);
    } else {
        (void)EntityFactory::createGoal(m_registry, m_levelData.goalPosition);
    }
}

void GameScene::spawnCurrentPlayer() {
    int idx = m_state->currentPlayer;
    Player& player = (idx == 0) ? m_player1 : m_player2;
    player.spawn(m_registry, m_spawnPoint, idx);
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
    if (m_state->gameOver || m_state->levelWon) return;

    // Countdown level timer
    m_state->levelTimer -= dt;
    if (m_state->levelTimer <= 0.0f) {
        m_state->levelTimer = 0.0f;
        m_state->gameOver = true;
        m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false, m_state->p1.score));
        return;
    }

    // ---- System tick order ----
    m_playerSystem.update(m_registry, dt, m_game.input(), m_game.events());
    m_enemyAISystem.update(m_registry, dt, m_game.events());
    m_physicsSystem.update(m_registry, dt);
    m_movementSystem.update(m_registry, dt);
    m_collisionSystem.update(m_registry, dt, m_game.events());
    m_powerUpSystem.update(m_registry, dt, m_game.events());
    m_animationSystem.update(m_registry, dt);

    // Update camera to follow the active player (or P1 in co-op)
    Player* cameraTarget = &m_player1;
    if (!m_state->coopMode && m_state->currentPlayer == 1) {
        cameraTarget = &m_player2;
    }

    if (cameraTarget->isValid(m_registry)) {
        const auto& pTransform = m_registry.get<TransformComponent>(cameraTarget->getEntity());
        m_camera.setTarget(pTransform.position);
    }
    m_camera.update(dt);

    // Apply camera offset to platform
    m_game.platform().setCameraOffset(m_camera.getViewOffset());

    // Check for player(s) falling into pit
    auto checkPit = [&](Player& player, int playerIdx) {
        if (!player.isValid(m_registry)) return;
        const auto& pos = m_registry.get<TransformComponent>(player.getEntity()).position;
        if (pos.y <= m_tileMap.getPixelHeight() + 100.0f) return;

        auto& ps = (playerIdx == 0) ? m_state->p1 : m_state->p2;
        --ps.lives;

        if (m_state->coopMode) {
            // Co-op: check if both players are out of lives
            if (m_state->p1.lives <= 0 && m_state->p2.lives <= 0) {
                m_state->gameOver = true;
                m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false,
                    m_state->p1.score + m_state->p2.score));
            } else if (ps.lives > 0) {
                player.respawn(m_registry, m_spawnPoint, playerIdx);
            } else {
                // This player is out — destroy entity
                if (player.isValid(m_registry)) {
                    m_registry.destroy(player.getEntity());
                }
            }
        } else if (m_state->numPlayers == 2) {
            // Alternating: player dies → switch to other player
            if (ps.lives <= 0) {
                // Check if both players are out
                if (m_state->p1.lives <= 0 && m_state->p2.lives <= 0) {
                    m_state->gameOver = true;
                    m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false,
                        m_state->p1.score + m_state->p2.score));
                } else {
                    switchTurn();
                }
            } else {
                player.respawn(m_registry, m_spawnPoint, playerIdx);
                m_camera.setTarget(m_spawnPoint);
            }
        } else {
            // 1P mode
            if (ps.lives <= 0) {
                m_state->gameOver = true;
                m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false, ps.score));
            } else {
                player.respawn(m_registry, m_spawnPoint, playerIdx);
                m_camera.setTarget(m_spawnPoint);
            }
        }
    };

    if (m_state->coopMode) {
        checkPit(m_player1, 0);
        checkPit(m_player2, 1);
    } else {
        Player& activePlayer = (m_state->currentPlayer == 0) ? m_player1 : m_player2;
        checkPit(activePlayer, m_state->currentPlayer);
    }
}

void GameScene::render(IPlatform& platform) {
    // Parallax background
    m_parallax.render(platform, m_camera.getViewOffset());

    // Tilemap
    m_tileMap.render(platform, m_camera.getViewOffset());

    // Entities via RenderSystem
    m_renderSystem.update(m_registry, platform);
}

// =============================================================================
// Event handlers
// =============================================================================

void GameScene::onPlayerDied(const PlayerDiedEvent& event) {
    auto& ps = (event.playerIndex == 0) ? m_state->p1 : m_state->p2;
    --ps.lives;

    if (m_state->coopMode) {
        if (m_state->p1.lives <= 0 && m_state->p2.lives <= 0) {
            m_state->gameOver = true;
            m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false,
                m_state->p1.score + m_state->p2.score));
        } else if (ps.lives > 0) {
            Player& player = (event.playerIndex == 0) ? m_player1 : m_player2;
            player.respawn(m_registry, m_spawnPoint, event.playerIndex);
        }
    } else if (m_state->numPlayers == 2) {
        // Alternating mode
        if (m_state->p1.lives <= 0 && m_state->p2.lives <= 0) {
            m_state->gameOver = true;
            m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false,
                m_state->p1.score + m_state->p2.score));
        } else if (ps.lives <= 0) {
            // This player is out, switch to other
            switchTurn();
        } else {
            Player& player = (m_state->currentPlayer == 0) ? m_player1 : m_player2;
            player.respawn(m_registry, m_spawnPoint, m_state->currentPlayer);
            m_camera.setTarget(m_spawnPoint);
        }
    } else {
        if (ps.lives <= 0) {
            m_state->gameOver = true;
            m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false, ps.score));
        } else {
            m_player1.respawn(m_registry, m_spawnPoint, 0);
            m_camera.setTarget(m_spawnPoint);
        }
    }
}

void GameScene::onPlayerHurt(const PlayerHurtEvent& /*event*/) {
    m_camera.addShake(6.0f);
}

void GameScene::onCoinCollected(const CoinCollectedEvent& event) {
    auto& ps = (event.playerIndex == 0) ? m_state->p1 : m_state->p2;
    ps.score = Math::safeAdd(ps.score, event.value);
    ++ps.coins;

    // 100 coins = extra life
    if (ps.coins >= Config::COIN_EXTRA_LIFE) {
        ps.coins -= Config::COIN_EXTRA_LIFE;
        ++ps.lives;
        AudioManager::instance().playSound("one_up");
    }
}

void GameScene::onEnemyKilled(const EnemyKilledEvent& event) {
    auto& ps = m_state->current();
    ps.score = Math::safeAdd(ps.score, event.scoreValue > 0 ? event.scoreValue : Config::ENEMY_STOMP_VALUE);
}

void GameScene::onLevelComplete(const LevelCompleteEvent& /*event*/) {
    auto& ps = m_state->current();
    // Time bonus: remaining seconds * 50 points
    int timeBonus = static_cast<int>(m_state->levelTimer) * 50;
    ps.score = Math::safeAdd(ps.score, timeBonus);
    m_state->levelWon = true;

    int totalScore = m_state->p1.score;
    if (m_state->numPlayers == 2) totalScore += m_state->p2.score;
    m_game.scenes().push(std::make_unique<GameOverScene>(m_game, true, totalScore));
}

void GameScene::onBlockHit(const BlockHitEvent& event) {
    auto& ps = (event.playerIndex == 0) ? m_state->p1 : m_state->p2;
    ps.score = Math::safeAdd(ps.score, Config::BLOCK_HIT_VALUE);
    AudioManager::instance().playSound("block_hit");
}

void GameScene::onFlagPoleGrabbed(const FlagPoleGrabbedEvent& event) {
    int flagScore = static_cast<int>(event.grabHeight * static_cast<float>(Config::FLAGPOLE_BASE_SCORE));
    auto& ps = (event.playerIndex == 0) ? m_state->p1 : m_state->p2;
    ps.score = Math::safeAdd(ps.score, std::max(100, flagScore));
}

// =============================================================================
// Alternating mode turn switching
// =============================================================================

void GameScene::switchTurn() {
    // Destroy current player entity
    Player& current = (m_state->currentPlayer == 0) ? m_player1 : m_player2;
    if (current.isValid(m_registry)) {
        m_registry.destroy(current.getEntity());
    }

    // Toggle to the other player
    m_state->currentPlayer = 1 - m_state->currentPlayer;

    // If the new player is also out of lives, skip to game over
    auto& newPs = m_state->current();
    if (newPs.lives <= 0) {
        m_state->gameOver = true;
        m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false,
            m_state->p1.score + m_state->p2.score));
        return;
    }

    // Spawn the new current player
    spawnCurrentPlayer();
    m_camera.setTarget(m_spawnPoint);

    // Show "Get Ready!" interstitial
    m_game.scenes().push(std::make_unique<GetReadyScene>(m_game, m_state));
}
