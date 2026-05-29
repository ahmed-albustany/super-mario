#include "core/ResourceManager.hpp"
#include "utils/Logger.hpp"
#include "utils/SafeFileIO.hpp"
#include <nlohmann/json.hpp>

int ResourceManager::preload(const std::string& manifestPath,
                              const std::string& assetsRoot) {
    if (!m_platform) {
        LOG_ERROR("ResourceManager: platform not initialized");
        return -1;
    }

    // Read manifest via safe file I/O
    auto content = SafeIO::readFile(manifestPath);
    if (!content) {
        LOG_ERROR("ResourceManager: failed to read manifest");
        return -1;
    }

    int failures = 0;

    try {
        auto json = nlohmann::json::parse(*content);

        // Load textures
        if (json.contains("textures") && json["textures"].is_object()) {
            for (auto& [key, pathVal] : json["textures"].items()) {
                std::string fullPath = assetsRoot + pathVal.get<std::string>();
                if (!loadTexture(key, fullPath)) {
                    LOG_ERROR("Failed to preload texture: " << key);
                    ++failures;
                }
            }
        }

        // Load sounds
        if (json.contains("sounds") && json["sounds"].is_object()) {
            for (auto& [key, pathVal] : json["sounds"].items()) {
                std::string fullPath = assetsRoot + pathVal.get<std::string>();
                if (!loadSound(key, fullPath)) {
                    LOG_ERROR("Failed to preload sound: " << key);
                    ++failures;
                }
            }
        }

        // Load fonts
        if (json.contains("fonts") && json["fonts"].is_object()) {
            for (auto& [key, pathVal] : json["fonts"].items()) {
                std::string fullPath = assetsRoot + pathVal.get<std::string>();
                if (!loadFont(key, fullPath)) {
                    LOG_ERROR("Failed to preload font: " << key);
                    ++failures;
                }
            }
        }

    } catch (const nlohmann::json::exception& e) {
        LOG_ERROR("ResourceManager: JSON parse error in manifest: " << e.what());
        return -1;
    }

    LOG_INFO("ResourceManager: preload complete (" << failures << " failures)");
    return failures;
}
