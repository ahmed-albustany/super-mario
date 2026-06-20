#include "scenes/BootScene.hpp"
#include "scenes/MenuScene.hpp"
#include "core/Game.hpp"
#include "core/ResourceManager.hpp"
#include "core/GameConfig.hpp"
#include "utils/SafeFileIO.hpp"
#include "utils/Logger.hpp"
#include <nlohmann/json.hpp>

BootScene::BootScene(Game& game) : m_game(game) {}

void BootScene::onEnter() {
    LOG_INFO("BootScene: loading asset manifest");

    auto content = SafeIO::readFile("manifest.json");
    if (!content) {
        LOG_ERROR("BootScene: manifest.json not found — skipping preload");
        m_loadDone = true;
        return;
    }

    try {
        auto json = nlohmann::json::parse(*content);

        auto parseSection = [&](const std::string& section, const std::string& type) {
            if (!json.contains(section)) return;
            const auto& arr = json[section];
            if (arr.is_array()) {
                for (const auto& item : arr) {
                    if (item.is_object() &&
                        item.contains("key") && item["key"].is_string() &&
                        item.contains("path") && item["path"].is_string()) {
                        m_assets.push_back({type,
                                            item["key"].get<std::string>(),
                                            item["path"].get<std::string>()});
                    }
                }
            } else if (arr.is_object()) {
                // Legacy format: { "key": "path", ... }
                for (auto& [key, val] : arr.items()) {
                    if (val.is_string()) {
                        m_assets.push_back({type, key, val.get<std::string>()});
                    }
                }
            }
        };

        parseSection("textures", "texture");
        parseSection("sounds", "sound");
        parseSection("fonts", "font");
        // Note: music is loaded on-demand by AudioManager::playMusic() via
        // IPlatform::playMusic(path), not through ResourceManager handles.

    } catch (const nlohmann::json::exception& e) {
        LOG_ERROR("BootScene: manifest parse error: " << e.what());
        m_loadDone = true;
    }

    LOG_INFO("BootScene: " << m_assets.size() << " assets to load");
}

void BootScene::onExit() {}

void BootScene::handleInput(const InputManager& /*input*/) {}

void BootScene::update(float dt) {
    m_elapsed += dt;

    // Load a batch of assets per frame (up to 4 per frame to avoid long frames)
    int loaded = 0;
    while (m_loadIndex < m_assets.size() && loaded < 4) {
        const auto& entry = m_assets[m_loadIndex];
        auto& rm = ResourceManager::instance();
        std::string fullPath = "assets/" + entry.path;

        bool ok = false;
        if (entry.type == "texture") {
            ok = rm.loadTexture(entry.key, fullPath);
        } else if (entry.type == "sound") {
            ok = rm.loadSound(entry.key, fullPath);
        } else if (entry.type == "font") {
            ok = rm.loadFont(entry.key, fullPath);
        }

        if (!ok) {
            LOG_WARN("BootScene: failed to load " << entry.type << " '" << entry.key << "'");
            ++m_failures;
        }

        ++m_loadIndex;
        ++loaded;
    }

    if (m_loadIndex >= m_assets.size()) {
        m_loadDone = true;
    }

    // Transition to menu after loading + minimum display time
    if (m_loadDone && m_elapsed >= m_minDisplayTime) {
        LOG_INFO("BootScene: preload complete (" << m_failures << " failures)");
        m_game.scenes().replace(std::make_unique<MenuScene>(m_game));
    }
}

void BootScene::render(IPlatform& platform) {
    platform.clear(Color{10, 10, 20, 255});

    float screenW = static_cast<float>(Config::WINDOW_WIDTH);
    float screenH = static_cast<float>(Config::WINDOW_HEIGHT);

    // Title text
    auto font = ResourceManager::instance().getFont("main");

    // Progress bar background
    float barW = screenW * 0.6f;
    float barH = 20.0f;
    float barX = (screenW - barW) * 0.5f;
    float barY = screenH * 0.6f;

    platform.drawRect({barX, barY, barW, barH},
                      Color{40, 40, 60, 255},
                      Color{80, 80, 120, 255}, 2.0f);

    // Progress bar fill
    float progress = m_assets.empty() ? 1.0f
        : static_cast<float>(m_loadIndex) / static_cast<float>(m_assets.size());
    float fillW = (barW - 4.0f) * progress;

    platform.drawRect({barX + 2.0f, barY + 2.0f, fillW, barH - 4.0f},
                      Color{100, 200, 255, 255});

    // "Loading..." text (positioned above bar)
    if (font) {
        platform.drawText(*font, "Loading...",
                          {barX, barY - 36.0f}, 20, Color::White());

        // Asset count
        std::string countStr = std::to_string(m_loadIndex) + " / " + std::to_string(m_assets.size());
        platform.drawText(*font, countStr,
                          {barX + barW - 100.0f, barY - 36.0f}, 16,
                          Color{180, 180, 200, 255});
    }

    // Game title at top
    if (font) {
        platform.drawText(*font, Config::GAME_TITLE,
                          {screenW * 0.5f - 140.0f, screenH * 0.3f}, 36,
                          Color{78, 205, 196, 255});
    }
}
