#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/ResourceManager.hpp"
#include "scenes/BootScene.hpp"
#include "utils/SafeFileIO.hpp"
#include "utils/Logger.hpp"

#ifdef MARIO_WASM
static const std::string ASSETS_ROOT = "/assets/";
#else
static const std::string ASSETS_ROOT = "assets/";
#endif

Game::Game(std::unique_ptr<IPlatform> platform)
    : m_platform(std::move(platform))
{
    initSubsystems();
    LOG_INFO("Game initialized — " << Config::GAME_TITLE << " v" << Config::GAME_VERSION);
}

Game::~Game() {
    AudioManager::instance().shutdown(m_eventBus);
    ResourceManager::instance().unloadAll();
    LOG_INFO("Game destroyed");
}

void Game::initSubsystems() {
    // Set safe I/O root
    SafeIO::setRoot(ASSETS_ROOT);

    // Bind managers to platform
    ResourceManager::instance().init(*m_platform);
    m_inputManager.init(*m_platform);
    AudioManager::instance().init(*m_platform, m_eventBus);

    // Boot sequence: BootScene loads assets, then transitions to MenuScene
    m_sceneManager.push(std::make_unique<BootScene>(*this));
}

void Game::tick() {
    // ---- 1. Poll platform events & update delta time ----
    m_platform->pollEvents();

    // ---- 2. Process input ----
    m_inputManager.update();

    // ---- 3. Fixed-timestep update loop ----
    float dt = m_platform->getDeltaTime();

    // Cap to prevent spiral of death
    if (dt > Config::MAX_DELTA_TIME) {
        dt = Config::MAX_DELTA_TIME;
    }

    m_accumulator += dt;

    while (m_accumulator >= Config::FIXED_TIMESTEP) {
        m_sceneManager.handleInput(m_inputManager);
        m_sceneManager.update(Config::FIXED_TIMESTEP);
        AudioManager::instance().update(Config::FIXED_TIMESTEP);
        m_accumulator -= Config::FIXED_TIMESTEP;
    }

    // ---- 4. Apply deferred scene commands ----
    m_sceneManager.applyPendingCommands();

    // ---- 5. Render ----
    m_platform->clear(Color{20, 20, 30, 255}); // dark blue-gray background
    m_sceneManager.render(*m_platform);
    m_platform->display();
}

bool Game::isRunning() const {
    return m_platform->isRunning();
}
