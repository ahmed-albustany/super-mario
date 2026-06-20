#include "world/LevelLoader.hpp"
#include "utils/SafeFileIO.hpp"
#include "utils/Logger.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {

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
    auto content = SafeIO::readFile(relativePath);
    if (!content) {
        LOG_ERROR("LevelLoader: failed to read level file");
        return std::nullopt;
    }

    LevelData data;

    try {
        auto root = json::parse(*content);

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
        data.widthTiles  = safeGet<int>(meta, "width", 60);
        data.heightTiles = safeGet<int>(meta, "height", 20);
        data.music       = safeGet<std::string>(meta, "music", "");
        data.timeLimit   = safeGet<float>(meta, "time_limit", 300.0f);
        data.gravity     = safeGet<float>(meta, "gravity", 0.0f);
        data.background  = safeGet<std::string>(meta, "background", "bg_green");

        if (data.widthTiles <= 0 || data.widthTiles > 10000 ||
            data.heightTiles <= 0 || data.heightTiles > 1000) {
            LOG_ERROR("LevelLoader: level dimensions out of range");
            return std::nullopt;
        }

        if (meta.contains("background_layers") && meta["background_layers"].is_array()) {
            for (const auto& layer : meta["background_layers"]) {
                if (layer.is_string()) {
                    data.backgroundLayers.push_back(layer.get<std::string>());
                }
            }
        }

        // ================================================================
        // Parse tiles (16px tile size for Pixel Adventure)
        // ================================================================
        const int tileSize = 16;
        for (const auto& t : root["tiles"]) {
            if (!t.is_object()) continue;

            TileData td;
            td.x           = safeGet<int>(t, "x", 0);
            td.y           = safeGet<int>(t, "y", 0);
            td.id          = safeGet<int>(t, "id", 1);
            td.solid       = safeGet<bool>(t, "solid", false);
            td.destructible = safeGet<bool>(t, "destructible", false);

            if (td.x < 0 || td.x >= data.widthTiles ||
                td.y < 0 || td.y >= data.heightTiles) {
                continue;
            }

            data.tiles.push_back(td);
        }

        // ================================================================
        // Parse spawn points
        // ================================================================
        const auto& spawns = root["spawn_points"];

        // Player spawn
        if (spawns.contains("player") && spawns["player"].is_object()) {
            const auto& ps = spawns["player"];
            float rawX = safeGet<float>(ps, "x", 2.0f);
            float rawY = safeGet<float>(ps, "y", 17.0f);
            if (rawX < static_cast<float>(data.widthTiles) &&
                rawY < static_cast<float>(data.heightTiles)) {
                data.playerSpawn = {rawX * static_cast<float>(tileSize),
                                    rawY * static_cast<float>(tileSize)};
            } else {
                data.playerSpawn = {rawX, rawY};
            }
        }

        // ---- Fruits (new) ----
        if (spawns.contains("fruits") && spawns["fruits"].is_array()) {
            for (const auto& f : spawns["fruits"]) {
                if (!f.is_object()) continue;
                FruitSpawnData fsd;
                fsd.type = safeGet<std::string>(f, "type", "cherry");
                fsd.x    = safeGet<float>(f, "x", 0.0f) * static_cast<float>(tileSize);
                fsd.y    = safeGet<float>(f, "y", 0.0f) * static_cast<float>(tileSize);
                data.fruits.push_back(fsd);
            }
        }

        // ---- Traps (new) ----
        if (spawns.contains("traps") && spawns["traps"].is_array()) {
            for (const auto& t : spawns["traps"]) {
                if (!t.is_object()) continue;
                TrapSpawnData tsd;
                tsd.type      = safeGet<std::string>(t, "type", "spikes");
                tsd.x         = safeGet<float>(t, "x", 0.0f) * static_cast<float>(tileSize);
                tsd.y         = safeGet<float>(t, "y", 0.0f) * static_cast<float>(tileSize);
                tsd.speed     = safeGet<float>(t, "speed", 1.0f);
                tsd.onTime    = safeGet<float>(t, "on_time", 2.0f);
                tsd.offTime   = safeGet<float>(t, "off_time", 1.0f);
                tsd.strength  = safeGet<float>(t, "strength", 400.0f);
                tsd.chainLength = safeGet<float>(t, "chain_length", 8.0f);
                tsd.direction = safeGet<std::string>(t, "direction", "right");

                if (t.contains("path") && t["path"].is_array()) {
                    for (const auto& wp : t["path"]) {
                        if (wp.is_object()) {
                            float px = safeGet<float>(wp, "x", 0.0f);
                            float py = safeGet<float>(wp, "y", 0.0f);
                            tsd.path.push_back({px, py});
                        }
                    }
                }

                data.traps.push_back(tsd);
            }
        }

        // ---- Boxes (new) ----
        if (spawns.contains("boxes") && spawns["boxes"].is_array()) {
            for (const auto& b : spawns["boxes"]) {
                if (!b.is_object()) continue;
                BoxSpawnData bsd;
                bsd.type = safeGet<std::string>(b, "type", "box1");
                bsd.x    = safeGet<float>(b, "x", 0.0f) * static_cast<float>(tileSize);
                bsd.y    = safeGet<float>(b, "y", 0.0f) * static_cast<float>(tileSize);
                bsd.hits = safeGet<int>(b, "hits", 3);
                data.boxes.push_back(bsd);
            }
        }

        // ---- Checkpoints (new) ----
        if (spawns.contains("checkpoints") && spawns["checkpoints"].is_array()) {
            for (const auto& c : spawns["checkpoints"]) {
                if (!c.is_object()) continue;
                CheckpointSpawnData csd;
                csd.x = safeGet<float>(c, "x", 0.0f) * static_cast<float>(tileSize);
                csd.y = safeGet<float>(c, "y", 0.0f) * static_cast<float>(tileSize);
                data.checkpoints.push_back(csd);
            }
        }

        // ---- Legacy: enemies ----
        if (spawns.contains("enemies") && spawns["enemies"].is_array()) {
            for (const auto& e : spawns["enemies"]) {
                if (!e.is_object()) continue;
                EnemySpawnData esd;
                esd.type        = safeGet<std::string>(e, "type", "goomba");
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

        // ---- Legacy: collectibles ----
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

        // ---- Legacy: question blocks ----
        if (spawns.contains("question_blocks") && spawns["question_blocks"].is_array()) {
            for (const auto& q : spawns["question_blocks"]) {
                if (!q.is_object()) continue;
                QuestionBlockSpawnData qsd;
                qsd.x        = safeGet<float>(q, "x", 0.0f) * 32.0f;
                qsd.y        = safeGet<float>(q, "y", 0.0f) * 32.0f;
                qsd.contents = safeGet<std::string>(q, "contents", "coin");
                data.questionBlocks.push_back(qsd);
            }
        }

        // ---- Legacy: pipes ----
        if (spawns.contains("pipes") && spawns["pipes"].is_array()) {
            for (const auto& p : spawns["pipes"]) {
                if (!p.is_object()) continue;
                PipeSpawnData psd;
                psd.x         = safeGet<float>(p, "x", 0.0f) * 32.0f;
                psd.y         = safeGet<float>(p, "y", 0.0f) * 32.0f;
                psd.enterable = safeGet<bool>(p, "enterable", false);
                psd.destX     = safeGet<float>(p, "dest_x", 0.0f) * 32.0f;
                psd.destY     = safeGet<float>(p, "dest_y", 0.0f) * 32.0f;
                data.pipes.push_back(psd);
            }
        }

        // ================================================================
        // Parse goal
        // ================================================================
        if (root.contains("goal") && root["goal"].is_object()) {
            const auto& g = root["goal"];
            float gx = safeGet<float>(g, "x", 57.0f);
            float gy = safeGet<float>(g, "y", 17.0f);
            data.goalPosition = {gx * static_cast<float>(tileSize),
                                 gy * static_cast<float>(tileSize)};
            data.goalType = safeGet<std::string>(g, "type", "trophy");
            data.flagPoleHeight = safeGet<float>(g, "height", 288.0f);
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
             << data.fruits.size() << " fruits, "
             << data.traps.size() << " traps)");

    return data;
}
