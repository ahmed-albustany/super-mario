#include "world/LevelLoader.hpp"
#include "utils/SafeFileIO.hpp"
#include "utils/Logger.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {

/// @brief Safe accessor — returns default if key is missing or wrong type.
template<typename T>
T safeGet(const json& j, const std::string& key, const T& defaultVal) {
    if (j.contains(key)) {
        try {
            return j[key].get<T>();
        } catch (const json::exception&) {
            return defaultVal;
        }
    }
    return defaultVal;
}

} // anonymous namespace

std::optional<LevelData> LevelLoader::load(const std::string& relativePath) {
    // Read file via safe I/O
    auto content = SafeIO::readFile(relativePath);
    if (!content) {
        LOG_ERROR("LevelLoader: failed to read level file");
        return std::nullopt;
    }

    LevelData data;

    try {
        auto root = json::parse(*content);

        // ---- Validate required top-level sections ----
        if (!root.contains("meta") || !root["meta"].is_object()) {
            LOG_ERROR("LevelLoader: missing 'meta' section");
            return std::nullopt;
        }
        if (!root.contains("tiles") || !root["tiles"].is_array()) {
            LOG_ERROR("LevelLoader: missing 'tiles' array");
            return std::nullopt;
        }
        if (!root.contains("spawn_points") || !root["spawn_points"].is_object()) {
            LOG_ERROR("LevelLoader: missing 'spawn_points' section");
            return std::nullopt;
        }

        // ================================================================
        // Parse meta
        // ================================================================
        const auto& meta = root["meta"];

        data.name       = safeGet<std::string>(meta, "name", "Unnamed Level");
        data.theme      = safeGet<std::string>(meta, "theme", "default");
        data.widthTiles  = safeGet<int>(meta, "width", 200);
        data.heightTiles = safeGet<int>(meta, "height", 15);
        data.music       = safeGet<std::string>(meta, "music", "");
        data.timeLimit   = safeGet<float>(meta, "time_limit", 300.0f);
        data.gravity     = safeGet<float>(meta, "gravity", 0.0f);

        // Sanity bounds
        if (data.widthTiles <= 0 || data.widthTiles > 10000 ||
            data.heightTiles <= 0 || data.heightTiles > 1000) {
            LOG_ERROR("LevelLoader: level dimensions out of range");
            return std::nullopt;
        }

        // Background layers
        if (meta.contains("background_layers") && meta["background_layers"].is_array()) {
            for (const auto& layer : meta["background_layers"]) {
                if (layer.is_string()) {
                    data.backgroundLayers.push_back(layer.get<std::string>());
                }
            }
        }

        // ================================================================
        // Parse tiles
        // ================================================================
        for (const auto& t : root["tiles"]) {
            if (!t.is_object()) continue;

            TileData td;
            td.x           = safeGet<int>(t, "x", 0);
            td.y           = safeGet<int>(t, "y", 0);
            td.id          = safeGet<int>(t, "id", 1);
            td.solid       = safeGet<bool>(t, "solid", false);
            td.destructible = safeGet<bool>(t, "destructible", false);

            // Validate tile coords are within declared level bounds
            if (td.x < 0 || td.x >= data.widthTiles ||
                td.y < 0 || td.y >= data.heightTiles) {
                continue; // skip out-of-bounds tiles silently
            }

            data.tiles.push_back(td);
        }

        // ================================================================
        // Parse spawn points
        // ================================================================
        const auto& spawns = root["spawn_points"];

        // Player spawn (tile coordinates → pixels)
        if (spawns.contains("player") && spawns["player"].is_object()) {
            const auto& ps = spawns["player"];
            float rawX = safeGet<float>(ps, "x", 2.0f);
            float rawY = safeGet<float>(ps, "y", 12.0f);
            // Convert tile coords to pixel coords
            if (rawX < static_cast<float>(data.widthTiles) &&
                rawY < static_cast<float>(data.heightTiles)) {
                data.playerSpawn = {rawX * 32.0f, rawY * 32.0f};
            } else {
                // Already in pixel coordinates
                data.playerSpawn = {rawX, rawY};
            }
        }

        // Enemy spawns
        if (spawns.contains("enemies") && spawns["enemies"].is_array()) {
            for (const auto& e : spawns["enemies"]) {
                if (!e.is_object()) continue;

                EnemySpawnData esd;
                esd.type        = safeGet<std::string>(e, "type", "walker");
                esd.x           = safeGet<float>(e, "x", 0.0f) * 32.0f;
                esd.y           = safeGet<float>(e, "y", 0.0f) * 32.0f;
                esd.patrolLeft  = safeGet<float>(e, "patrol_left", esd.x / 32.0f - 3.0f) * 32.0f;
                esd.patrolRight = safeGet<float>(e, "patrol_right", esd.x / 32.0f + 3.0f) * 32.0f;
                esd.facing = 1;
                if (e.contains("facing")) {
                    std::string f = safeGet<std::string>(e, "facing", "right");
                    esd.facing = (f == "left") ? -1 : 1;
                }

                data.enemies.push_back(esd);
            }
        }

        // Collectible spawns
        if (spawns.contains("collectibles") && spawns["collectibles"].is_array()) {
            for (const auto& c : spawns["collectibles"]) {
                if (!c.is_object()) continue;

                CollectibleSpawnData csd;
                csd.type = safeGet<std::string>(c, "type", "coin");
                csd.x    = safeGet<float>(c, "x", 0.0f) * 32.0f;
                csd.y    = safeGet<float>(c, "y", 0.0f) * 32.0f;

                data.collectibles.push_back(csd);
            }
        }

        // ================================================================
        // Parse goal
        // ================================================================
        if (root.contains("goal") && root["goal"].is_object()) {
            const auto& g = root["goal"];
            data.goalPosition = {
                safeGet<float>(g, "x", 195.0f) * 32.0f,
                safeGet<float>(g, "y", 12.0f) * 32.0f
            };
            data.goalType = safeGet<std::string>(g, "type", "flagpole");
        }

    } catch (const json::exception& e) {
        LOG_ERROR("LevelLoader: JSON parse error: " << e.what());
        return std::nullopt;
    } catch (const std::exception& e) {
        LOG_ERROR("LevelLoader: unexpected error: " << e.what());
        return std::nullopt;
    }

    LOG_INFO("LevelLoader: loaded '" << data.name << "' ("
             << data.widthTiles << "x" << data.heightTiles << ", "
             << data.tiles.size() << " tiles, "
             << data.enemies.size() << " enemies, "
             << data.collectibles.size() << " collectibles)");

    return data;
}
