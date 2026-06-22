#include "scenes/GameScene.hpp"
#include "scenes/PauseScene.hpp"
#include "scenes/GameOverScene.hpp"
#include "scenes/VictoryScene.hpp"
#include "scenes/LevelCompleteScene.hpp"
#include "scenes/HUDScene.hpp"
#include "scenes/GetReadyScene.hpp"
#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/InputManager.hpp"
#include "core/ResourceManager.hpp"
#include "audio/AudioManager.hpp"
#include "core/SaveManager.hpp"
#include "entities/EntityFactory.hpp"
#include "ecs/Components.hpp"
#include "utils/Logger.hpp"
#include "utils/Math.hpp"

GameScene::GameScene(Game& game) : m_game(game) {}

void GameScene::setGameMode(GameMode mode) {
    m_state->mode = mode;
    m_state->currentPlayer = 0;
}

void GameScene::setStartLevel(int levelIndex) {
    m_state->currentLevel = levelIndex;
    if (levelIndex >= 0 && levelIndex < static_cast<int>(GameState::WORLD_NAMES.size())) {
        m_state->worldDisplay = GameState::WORLD_NAMES[static_cast<size_t>(levelIndex)];
    }
}

// =============================================================================
// Lifecycle
// =============================================================================

void GameScene::onEnter() {
    LOG_INFO("GameScene: entering (mode=" << static_cast<int>(m_state->mode) << ")");

    // Preserve mode and level index across state reset
    GameMode mode = m_state->mode;
    int startLevel = m_state->currentLevel;
    m_state = std::make_shared<GameState>();
    m_state->mode = mode;
    m_state->currentPlayer = 0;
    m_state->currentLevel = startLevel;
    m_state->worldDisplay = GameState::WORLD_NAMES[static_cast<size_t>(startLevel)];

    // Apply character picks from character select screen
    const auto& picks = m_game.characterPicks();
    for (int i = 0; i < 4; ++i) {
        m_state->players[static_cast<size_t>(i)].playerIndex = picks[static_cast<size_t>(i)];
        m_state->players[static_cast<size_t>(i)].characterType = static_cast<CharacterType>(picks[static_cast<size_t>(i)]);
    }

    // Initialize lives for active players
    int numActive = m_state->numActivePlayers();
    for (int i = 0; i < numActive; ++i) {
        m_state->players[static_cast<size_t>(i)].lives = Config::DEFAULT_LIVES;
    }

    // Subscribe to events
    auto& bus = m_game.events();
    m_subPlayerDied = bus.subscribe<PlayerDiedEvent>(
        [this](const PlayerDiedEvent& e) { onPlayerDied(e); });
    m_subFruitCollected = bus.subscribe<FruitCollectedEvent>(
        [this](const FruitCollectedEvent& e) { onFruitCollected(e); });
    m_subBoxHit = bus.subscribe<BoxHitEvent>(
        [this](const BoxHitEvent& e) { onBoxHit(e); });
    m_subBoxBreak = bus.subscribe<BoxBreakEvent>(
        [this](const BoxBreakEvent& e) { onBoxBreak(e); });
    m_subTrapDeath = bus.subscribe<TrapDeathEvent>(
        [this](const TrapDeathEvent& e) { onTrapDeath(e); });
    m_subCheckpoint = bus.subscribe<CheckpointActivatedEvent>(
        [this](const CheckpointActivatedEvent& e) { onCheckpointActivated(e); });
    m_subLevelComplete = bus.subscribe<LevelCompleteEvent>(
        [this](const LevelCompleteEvent& e) { onLevelComplete(e); });

    // Legacy event subscriptions (kept for backward compatibility)
    m_subCoin = bus.subscribe<CoinCollectedEvent>(
        [this](const CoinCollectedEvent& e) {
            auto& ps = m_state->players[static_cast<size_t>(e.playerIndex)];
            ps.score = Math::safeAdd(ps.score, e.value);
        });
    m_subEnemyKilled = bus.subscribe<EnemyKilledEvent>(
        [this](const EnemyKilledEvent& e) {
            auto& ps = m_state->current();
            ps.score = Math::safeAdd(ps.score, e.scoreValue > 0 ? e.scoreValue : Config::ENEMY_STOMP_VALUE);
        });
    m_subBlockHit = bus.subscribe<BlockHitEvent>(
        [this](const BlockHitEvent& e) {
            auto& ps = m_state->players[static_cast<size_t>(e.playerIndex)];
            ps.score = Math::safeAdd(ps.score, Config::BLOCK_HIT_VALUE);
        });
    m_subFlagPole = bus.subscribe<FlagPoleGrabbedEvent>(
        [this](const FlagPoleGrabbedEvent& e) {
            int flagScore = std::max(100, static_cast<int>(e.grabHeight * static_cast<float>(Config::FLAGPOLE_BASE_SCORE)));
            auto& ps = m_state->players[static_cast<size_t>(e.playerIndex)];
            ps.score = Math::safeAdd(ps.score, flagScore);
        });
    m_subPowerUp = bus.subscribe<PlayerPowerUpEvent>(
        [this](const PlayerPowerUpEvent& e) {
            if (e.powerType == "1up") {
                auto& ps = m_state->players[static_cast<size_t>(e.playerIndex)];
                ++ps.lives;
                AudioManager::instance().playSound("one_up");
            }
        });
    m_subPlayerHurt = bus.subscribe<PlayerHurtEvent>(
        [this](const PlayerHurtEvent&) { m_camera.addShake(6.0f); });
    m_subPlayerLanded = bus.subscribe<PlayerLandedEvent>(
        [this](const PlayerLandedEvent& e) {
            // Spawn dust puff at player's feet on landing
            int idx = e.playerIndex;
            auto& player = m_players[static_cast<size_t>(idx)];
            if (player.isValid(m_registry)) {
                auto pos = m_registry.get<TransformComponent>(player.getEntity()).position;
                pos.y += 24.0f; // feet position
                EntityFactory::createParticleEffect(m_registry, pos,
                    ParticleEmitterComponent::Effect::StompPoof);
            }
        });

    // Load current level
    const auto& levelPath = GameState::LEVEL_PATHS[static_cast<size_t>(m_state->currentLevel)];
    loadLevel(levelPath);

    // Push HUD as overlay on top of us
    m_game.scenes().push(std::make_unique<HUDScene>(m_game, m_state));

    // Start music
    const std::string& music = m_levelData.music;
    if (!music.empty()) {
        std::string musicKey = music;
        auto dotPos = musicKey.rfind('.');
        if (dotPos != std::string::npos) musicKey = musicKey.substr(0, dotPos);
        AudioManager::instance().playMusic(musicKey, true);
    } else {
        AudioManager::instance().playMusic("overworld_theme", true);
    }
}

void GameScene::onExit() {
    AudioManager::instance().stopMusic();

    // Pop the HUD overlay
    auto& sm = m_game.scenes();
    if (sm.current() && sm.current()->name() == "HUDScene") {
        sm.pop();
    }

    auto& bus = m_game.events();
    bus.unsubscribe<PlayerDiedEvent>(m_subPlayerDied);
    bus.unsubscribe<FruitCollectedEvent>(m_subFruitCollected);
    bus.unsubscribe<BoxHitEvent>(m_subBoxHit);
    bus.unsubscribe<BoxBreakEvent>(m_subBoxBreak);
    bus.unsubscribe<TrapDeathEvent>(m_subTrapDeath);
    bus.unsubscribe<CheckpointActivatedEvent>(m_subCheckpoint);
    bus.unsubscribe<LevelCompleteEvent>(m_subLevelComplete);
    bus.unsubscribe<CoinCollectedEvent>(m_subCoin);
    bus.unsubscribe<EnemyKilledEvent>(m_subEnemyKilled);
    bus.unsubscribe<BlockHitEvent>(m_subBlockHit);
    bus.unsubscribe<FlagPoleGrabbedEvent>(m_subFlagPole);
    bus.unsubscribe<PlayerPowerUpEvent>(m_subPowerUp);
    bus.unsubscribe<PlayerHurtEvent>(m_subPlayerHurt);
    bus.unsubscribe<PlayerLandedEvent>(m_subPlayerLanded);

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

    // Parallax backgrounds — try Pixel Adventure backgrounds first, then legacy
    m_parallax.clear();
    auto& rm = ResourceManager::instance();

    // Try loading the level-specific background, then fall back to legacy
    bool bgLoaded = false;
    if (!m_levelData.background.empty()) {
        auto bg = rm.getTexture(m_levelData.background);
        if (bg) {
            // Pixel Adventure backgrounds are 64x64 tiles — pass actual size so
            // the parallax renderer can tile them across the full screen.
            m_parallax.addDefaultLayers(
                *bg, 64.0f, 64.0f,
                *bg, 64.0f, 64.0f,
                *bg, 64.0f, 64.0f
            );
            bgLoaded = true;
        }
    }

    if (!bgLoaded) {
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
    }

    m_spawnPoint = m_levelData.playerSpawn;
    spawnEntities();

    // Grace period: skip pit detection for 1 second after level load
    m_spawnGraceTimer = 1.0f;

    LOG_INFO("GameScene: level '" << m_levelData.name << "' loaded"
             << " | spawn=(" << m_spawnPoint.x << "," << m_spawnPoint.y << ")"
             << " | pixelH=" << m_tileMap.getPixelHeight()
             << " | tiles=" << m_levelData.tiles.size());
}

void GameScene::spawnEntities() {
    // ---- Players ----
    spawnPlayers();

    // ---- Fruits ----
    for (const auto& fsd : m_levelData.fruits) {
        Vec2f pos{fsd.x, fsd.y};
        (void)EntityFactory::createFruitByName(m_registry, pos, fsd.type);
    }

    // ---- Traps ----
    for (const auto& tsd : m_levelData.traps) {
        Vec2f pos{tsd.x, tsd.y};

        if (tsd.type == "moving_platform" && !tsd.path.empty()) {
            (void)EntityFactory::createMovingPlatform(m_registry, pos, tsd.path, tsd.speed);
        } else if (tsd.type == "falling_platform") {
            (void)EntityFactory::createFallingPlatform(m_registry, pos);
        } else if (tsd.type == "spiked_ball") {
            (void)EntityFactory::createSpikedBall(m_registry, pos, tsd.chainLength);
        } else if (tsd.type == "fan") {
            (void)EntityFactory::createFan(m_registry, pos, tsd.strength);
        } else if (tsd.type == "arrow") {
            (void)EntityFactory::createArrow(m_registry, pos, tsd.direction);
        } else if (tsd.type == "fire") {
            (void)EntityFactory::createFire(m_registry, pos, tsd.onTime, tsd.offTime);
        } else if (tsd.type == "saw") {
            auto e = EntityFactory::createTrap(m_registry, pos, TrapType::Saw, tsd.speed);
            // If saw has a path, set it up
            if (!tsd.path.empty() && m_registry.all_of<TrapComponent>(e)) {
                auto& tc = m_registry.get<TrapComponent>(e);
                tc.path = tsd.path;
            }
        } else if (tsd.type == "spike_head") {
            (void)EntityFactory::createTrap(m_registry, pos, TrapType::SpikeHead, tsd.speed);
        } else if (tsd.type == "rock_head") {
            (void)EntityFactory::createTrap(m_registry, pos, TrapType::RockHead, tsd.speed);
        } else if (tsd.type == "spikes") {
            (void)EntityFactory::createTrap(m_registry, pos, TrapType::Spikes);
        } else if (tsd.type == "trampoline") {
            (void)EntityFactory::createTrap(m_registry, pos, TrapType::Trampoline);
        } else {
            // Generic trap
            (void)EntityFactory::createTrap(m_registry, pos, TrapType::Spikes);
        }
    }

    // ---- Boxes ----
    for (const auto& bsd : m_levelData.boxes) {
        Vec2f pos{bsd.x, bsd.y};
        (void)EntityFactory::createBox(m_registry, pos, bsd.type, bsd.hits);
    }

    // ---- Checkpoints ----
    for (const auto& csd : m_levelData.checkpoints) {
        Vec2f pos{csd.x, csd.y};
        (void)EntityFactory::createCheckpoint(m_registry, pos);
    }

    // ---- Goal (trophy) ----
    if (m_levelData.goalType == "trophy") {
        (void)EntityFactory::createTrophy(m_registry, m_levelData.goalPosition);
    } else if (m_levelData.goalType == "flagpole") {
        (void)EntityFactory::createFlagPole(m_registry, m_levelData.goalPosition,
                                            m_levelData.flagPoleHeight);
    } else {
        (void)EntityFactory::createTrophy(m_registry, m_levelData.goalPosition);
    }

    // ---- Legacy: enemies ----
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

    // ---- Legacy: collectibles ----
    for (const auto& csd : m_levelData.collectibles) {
        Vec2f pos{csd.x, csd.y};
        if (csd.type == "mushroom")           (void)EntityFactory::createMushroom(m_registry, pos);
        else if (csd.type == "fire_flower")   (void)EntityFactory::createFireFlower(m_registry, pos);
        else if (csd.type == "star")          (void)EntityFactory::createStar(m_registry, pos);
        else if (csd.type == "1up")           (void)EntityFactory::createOneUp(m_registry, pos);
        else                                  (void)EntityFactory::createCoin(m_registry, pos);
    }

    // ---- Legacy: question blocks ----
    for (const auto& qsd : m_levelData.questionBlocks) {
        Vec2f pos{qsd.x, qsd.y};
        CollectibleType contents = CollectibleType::Coin;
        if (qsd.contents == "mushroom")         contents = CollectibleType::Mushroom;
        else if (qsd.contents == "fire_flower") contents = CollectibleType::FireFlower;
        else if (qsd.contents == "star")        contents = CollectibleType::Star;
        else if (qsd.contents == "1up")         contents = CollectibleType::OneUp;
        (void)EntityFactory::createQuestionBlock(m_registry, pos, contents);
    }

    // ---- Legacy: pipes ----
    for (const auto& psd : m_levelData.pipes) {
        Vec2f pos{psd.x, psd.y};
        Vec2f dest{psd.destX, psd.destY};
        (void)EntityFactory::createPipe(m_registry, pos, psd.enterable, dest);
    }
}

void GameScene::spawnPlayers() {
    int numSim = numSimultaneousPlayers();
    float spacing = 24.0f;

    for (int i = 0; i < numSim; ++i) {
        // In alternating mode, spawn the current player (0 or 1), not always index 0
        int playerIdx = (m_state->mode == GameMode::Alt2P) ? m_state->currentPlayer : i;

        Vec2f spawnPos = m_spawnPoint;

        // In simultaneous modes, offset players horizontally
        if (numSim > 1) {
            spawnPos.x += static_cast<float>(i) * spacing;
        }

        // Use checkpoint position if available
        auto& ps = m_state->players[static_cast<size_t>(playerIdx)];
        if (ps.checkpointPos.x > 0.0f || ps.checkpointPos.y > 0.0f) {
            spawnPos = ps.checkpointPos;
            if (numSim > 1) {
                spawnPos.x += static_cast<float>(i) * spacing;
            }
        }

        if (ps.isAlive && ps.lives > 0) {
            // Use the character index from player state (set by character select)
            int charIdx = ps.playerIndex;
            m_players[static_cast<size_t>(playerIdx)].spawn(m_registry, spawnPos, charIdx);
            LOG_INFO("GameScene: spawned p" << playerIdx << " (char=" << charIdx << ") at (" << spawnPos.x << "," << spawnPos.y << ")"
                     << " lives=" << ps.lives
                     << " valid=" << m_players[static_cast<size_t>(playerIdx)].isValid(m_registry));
        } else {
            LOG_WARN("GameScene: SKIPPED spawn p" << playerIdx
                     << " isAlive=" << ps.isAlive << " lives=" << ps.lives);
        }
    }
}

void GameScene::respawnPlayer(int playerIndex) {
    auto& ps = m_state->players[static_cast<size_t>(playerIndex)];
    Vec2f respawnPos = (ps.checkpointPos.x > 0.0f || ps.checkpointPos.y > 0.0f)
                       ? ps.checkpointPos : m_spawnPoint;

    int charIdx = ps.playerIndex; // character index from character select
    m_players[static_cast<size_t>(playerIndex)].respawn(m_registry, respawnPos, charIdx);
    m_spawnGraceTimer = 1.0f; // Reset grace period on respawn
}

int GameScene::numSimultaneousPlayers() const {
    switch (m_state->mode) {
        case GameMode::Solo:   return 1;
        case GameMode::Alt2P:  return 1; // only one on screen at a time
        case GameMode::Coop2P: return 2;
        case GameMode::Coop4P: return 4;
        case GameMode::VS4P:   return 4;
    }
    return 1;
}

// =============================================================================
// Input / Update / Render
// =============================================================================

void GameScene::handleInput(const InputManager& input) {
    if (input.isJustPressed(Action::Pause)) {
        m_game.scenes().push(std::make_unique<PauseScene>(m_game, m_state->mode, m_state->currentLevel));
    }
}

void GameScene::update(float dt) {
    if (m_state->gameOver || m_state->levelWon) return;

    // Tick spawn grace timer
    if (m_spawnGraceTimer > 0.0f) {
        m_spawnGraceTimer -= dt;
    }

    // Countdown level timer
    m_state->levelTimer -= dt;
    if (m_state->levelTimer <= 0.0f) {
        m_state->levelTimer = 0.0f;
        m_state->gameOver = true;
        LOG_INFO("GameScene: GAME OVER — timer expired");
        int totalScore = 0;
        for (int i = 0; i < m_state->numActivePlayers(); ++i) {
            totalScore = Math::safeAdd(totalScore, m_state->players[static_cast<size_t>(i)].score);
        }
        m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false, totalScore, m_state->mode));
        return;
    }

    // ---- System tick order ----
    m_playerSystem.update(m_registry, dt, m_game.input(), m_game.events());
    m_enemyAISystem.update(m_registry, dt, m_game.events());
    m_trapSystem.update(m_registry, dt, m_game.events());
    m_fruitSystem.update(m_registry, dt, m_game.events());
    m_physicsSystem.update(m_registry, dt);
    m_movementSystem.update(m_registry, dt);
    m_collisionSystem.update(m_registry, dt, m_game.events());
    m_powerUpSystem.update(m_registry, dt, m_game.events());
    m_animationSystem.update(m_registry, dt);

    // Update camera to follow P1 (or the active player in alternating mode)
    int cameraPlayerIdx = (m_state->mode == GameMode::Alt2P) ? m_state->currentPlayer : 0;
    auto& cameraPlayer = m_players[static_cast<size_t>(cameraPlayerIdx)];
    if (cameraPlayer.isValid(m_registry)) {
        const auto& pTransform = m_registry.get<TransformComponent>(cameraPlayer.getEntity());
        m_camera.setTarget(pTransform.position);
    }
    m_camera.update(dt);
    m_game.platform().setCameraOffset(m_camera.getViewOffset());

    // Check for player(s) falling into pit (skip during spawn grace period)
    if (m_spawnGraceTimer > 0.0f) return;

    int numSim = numSimultaneousPlayers();
    for (int i = 0; i < numSim; ++i) {
        int playerIdx = (m_state->mode == GameMode::Alt2P) ? m_state->currentPlayer : i;
        auto& player = m_players[static_cast<size_t>(playerIdx)];
        if (!player.isValid(m_registry)) continue;

        const auto& pos = m_registry.get<TransformComponent>(player.getEntity()).position;
        if (pos.y <= m_tileMap.getPixelHeight() + 100.0f) continue;

        LOG_INFO("GameScene: PIT DEATH p" << playerIdx
                 << " y=" << pos.y
                 << " threshold=" << (m_tileMap.getPixelHeight() + 100.0f));
        // Player fell into pit
        auto& ps = m_state->players[static_cast<size_t>(playerIdx)];
        --ps.lives;

        // VS mode death penalty
        if (m_state->isVSMode()) {
            ps.score = std::max(0, ps.score - Config::VS_DEATH_PENALTY);
        }

        if (m_state->isSimultaneous()) {
            // Check if all simultaneous players are out
            bool allDead = true;
            for (int j = 0; j < numSim; ++j) {
                if (m_state->players[static_cast<size_t>(j)].lives > 0) {
                    allDead = false;
                    break;
                }
            }
            if (allDead) {
                m_state->gameOver = true;
                int totalScore = 0;
                for (int j = 0; j < numSim; ++j) {
                    totalScore = Math::safeAdd(totalScore, m_state->players[static_cast<size_t>(j)].score);
                }
                m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false, totalScore, m_state->mode));
            } else if (ps.lives > 0) {
                respawnPlayer(playerIdx);
            } else {
                ps.isAlive = false;
                if (player.isValid(m_registry)) {
                    m_registry.destroy(player.getEntity());
                }
            }
        } else if (m_state->mode == GameMode::Alt2P) {
            // Check if both alternating players are out
            if (m_state->players[0].lives <= 0 && m_state->players[1].lives <= 0) {
                m_state->gameOver = true;
                int totalScore = m_state->players[0].score + m_state->players[1].score;
                m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false, totalScore, m_state->mode));
            } else if (ps.lives <= 0) {
                switchTurn();
            } else {
                respawnPlayer(playerIdx);
                m_camera.setTarget(m_spawnPoint);
            }
        } else {
            // Solo
            if (ps.lives <= 0) {
                m_state->gameOver = true;
                m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false, ps.score, m_state->mode));
            } else {
                respawnPlayer(playerIdx);
                m_camera.setTarget(m_spawnPoint);
            }
        }

        // Only process one pit death per frame in alternating mode
        if (m_state->mode == GameMode::Alt2P) break;
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
    int idx = event.playerIndex;
    auto& ps = m_state->players[static_cast<size_t>(idx)];
    LOG_INFO("GameScene: PlayerDiedEvent p" << idx << " lives=" << ps.lives << "->(" << (ps.lives-1) << ")");
    --ps.lives;

    // VS mode death penalty
    if (m_state->isVSMode()) {
        ps.score = std::max(0, ps.score - Config::VS_DEATH_PENALTY);
    }

    if (m_state->isSimultaneous()) {
        bool allDead = true;
        int numSim = numSimultaneousPlayers();
        for (int j = 0; j < numSim; ++j) {
            if (m_state->players[static_cast<size_t>(j)].lives > 0) {
                allDead = false;
                break;
            }
        }
        if (allDead) {
            m_state->gameOver = true;
            int totalScore = 0;
            for (int j = 0; j < numSim; ++j) {
                totalScore = Math::safeAdd(totalScore, m_state->players[static_cast<size_t>(j)].score);
            }
            m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false, totalScore, m_state->mode));
        } else if (ps.lives > 0) {
            respawnPlayer(idx);
        } else {
            ps.isAlive = false;
        }
    } else if (m_state->mode == GameMode::Alt2P) {
        if (m_state->players[0].lives <= 0 && m_state->players[1].lives <= 0) {
            m_state->gameOver = true;
            m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false,
                m_state->players[0].score + m_state->players[1].score, m_state->mode));
        } else if (ps.lives <= 0) {
            switchTurn();
        } else {
            respawnPlayer(idx);
            m_camera.setTarget(m_spawnPoint);
        }
    } else {
        // Solo
        if (ps.lives <= 0) {
            m_state->gameOver = true;
            m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false, ps.score, m_state->mode));
        } else {
            respawnPlayer(idx);
            m_camera.setTarget(m_spawnPoint);
        }
    }
}

void GameScene::onFruitCollected(const FruitCollectedEvent& event) {
    auto& ps = m_state->players[static_cast<size_t>(event.playerIndex)];
    ps.score = Math::safeAdd(ps.score, event.value);
    ++ps.fruitsCollected;

    // Extra life every EXTRA_LIFE_SCORE points
    int prevLives = ps.score - event.value;
    if (ps.score / Config::EXTRA_LIFE_SCORE > prevLives / Config::EXTRA_LIFE_SCORE) {
        ++ps.lives;
        AudioManager::instance().playSound("one_up");
    }

    AudioManager::instance().playSound("fruit_collect");
}

void GameScene::onBoxHit(const BoxHitEvent& event) {
    AudioManager::instance().playSound("block_hit");
    m_camera.addShake(3.0f);
    (void)event;
}

void GameScene::onBoxBreak(const BoxBreakEvent& event) {
    // Spawn the fruit at the box position
    if (!event.fruitSpawned.empty()) {
        (void)EntityFactory::createFruitByName(m_registry, event.position, event.fruitSpawned);
    }
    AudioManager::instance().playSound("block_break");
}

void GameScene::onTrapDeath(const TrapDeathEvent& event) {
    // Death is handled via PlayerDiedEvent — this is for visual feedback
    m_camera.addShake(8.0f);
    (void)event;
}

void GameScene::onCheckpointActivated(const CheckpointActivatedEvent& event) {
    // Update checkpoint for the player who activated it
    auto& ps = m_state->players[static_cast<size_t>(event.playerIndex)];
    ps.checkpointPos = event.position;

    // In co-op modes, all players share the checkpoint
    if (m_state->isSimultaneous()) {
        int numSim = numSimultaneousPlayers();
        for (int i = 0; i < numSim; ++i) {
            m_state->players[static_cast<size_t>(i)].checkpointPos = event.position;
        }
    }

    AudioManager::instance().playSound("checkpoint");
}

void GameScene::onLevelComplete(const LevelCompleteEvent& event) {
    // Time bonus: remaining seconds * 50 points
    int timeBonus = static_cast<int>(m_state->levelTimer) * 50;

    if (m_state->isSimultaneous()) {
        auto& ps = m_state->players[static_cast<size_t>(event.playerIndex)];
        ps.score = Math::safeAdd(ps.score, timeBonus);

        if (m_state->isVSMode()) {
            ps.score *= Config::TROPHY_SCORE_MULTIPLIER;
        }
    } else {
        auto& ps = m_state->current();
        ps.score = Math::safeAdd(ps.score, timeBonus);
    }

    m_state->levelWon = true;

    // Persist progress
    {
        auto& sm = SaveManager::instance();
        SaveData sd = sm.load();
        int lvl = m_state->currentLevel;
        if (lvl >= 0 && lvl < static_cast<int>(sd.levelCompleted.size())) {
            sd.levelCompleted[static_cast<size_t>(lvl)] = true;
        }
        if (lvl + 1 > sd.highestLevel) sd.highestLevel = lvl + 1;
        int totalScore = 0;
        for (int i = 0; i < m_state->numActivePlayers(); ++i) {
            totalScore += m_state->players[static_cast<size_t>(i)].score;
        }
        if (totalScore > sd.highScore) sd.highScore = totalScore;
        sm.save(sd);
    }

    // Show level complete screen, then advance
    m_game.scenes().push(std::make_unique<LevelCompleteScene>(
        m_game, m_state, [this]() { advanceLevel(); }));
}

// =============================================================================
// Level progression
// =============================================================================

void GameScene::advanceLevel() {
    if (m_state->hasNextLevel()) {
        ++m_state->currentLevel;
        m_state->worldDisplay = GameState::WORLD_NAMES[static_cast<size_t>(m_state->currentLevel)];
        m_state->levelWon = false;
        m_state->levelTimer = 300.0f;

        // Reset checkpoints for new level
        for (auto& ps : m_state->players) {
            ps.checkpointPos = {0.0f, 0.0f};
        }

        m_registry.clear();
        loadLevel(GameState::LEVEL_PATHS[static_cast<size_t>(m_state->currentLevel)]);

        std::string musicKey = m_levelData.music;
        if (musicKey.size() > 4 && musicKey.substr(musicKey.size() - 4) == ".ogg")
            musicKey = musicKey.substr(0, musicKey.size() - 4);
        AudioManager::instance().playMusic(musicKey);
    } else {
        m_game.scenes().push(std::make_unique<VictoryScene>(m_game, m_state));
    }
}

// =============================================================================
// Alternating mode turn switching
// =============================================================================

void GameScene::switchTurn() {
    // Destroy current player entity
    auto& current = m_players[static_cast<size_t>(m_state->currentPlayer)];
    if (current.isValid(m_registry)) {
        m_registry.destroy(current.getEntity());
    }

    // Toggle to the other player
    m_state->currentPlayer = 1 - m_state->currentPlayer;

    // If the new player is also out of lives, game over
    auto& newPs = m_state->current();
    if (newPs.lives <= 0) {
        m_state->gameOver = true;
        m_game.scenes().push(std::make_unique<GameOverScene>(m_game, false,
            m_state->players[0].score + m_state->players[1].score, m_state->mode));
        return;
    }

    // Spawn the new current player
    respawnPlayer(m_state->currentPlayer);
    m_camera.setTarget(m_spawnPoint);

    // Show "Get Ready!" interstitial
    m_game.scenes().push(std::make_unique<GetReadyScene>(m_game, m_state));
}
