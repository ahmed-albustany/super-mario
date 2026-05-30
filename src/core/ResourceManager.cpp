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

        // Helper lambda — handles both array [{key, path}] and object {key: path} formats
        auto loadSection = [&](const std::string& section, auto loadFn) {
            if (!json.contains(section)) return;
            const auto& val = json[section];

            if (val.is_array()) {
                for (const auto& item : val) {
                    if (item.is_object() &&
                        item.contains("key") && item["key"].is_string() &&
                        item.contains("path") && item["path"].is_string()) {
                        std::string key = item["key"].get<std::string>();
                        std::string fullPath = assetsRoot + item["path"].get<std::string>();
                        if (!(this->*loadFn)(key, fullPath)) {
                            LOG_ERROR("Failed to preload: " << key);
                            ++failures;
                        }
                    }
                }
            } else if (val.is_object()) {
                for (auto& [key, pathVal] : val.items()) {
                    if (pathVal.is_string()) {
                        std::string fullPath = assetsRoot + pathVal.get<std::string>();
                        if (!(this->*loadFn)(key, fullPath)) {
                            LOG_ERROR("Failed to preload: " << key);
                            ++failures;
                        }
                    }
                }
            }
        };

        loadSection("textures", &ResourceManager::loadTexture);
        loadSection("sounds",   &ResourceManager::loadSound);
        loadSection("fonts",    &ResourceManager::loadFont);

    } catch (const nlohmann::json::exception& e) {
        LOG_ERROR("ResourceManager: JSON parse error in manifest: " << e.what());
        return -1;
    }

    LOG_INFO("ResourceManager: preload complete (" << failures << " failures)");
    return failures;
}
