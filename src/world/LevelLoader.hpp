#pragma once

#include <string>
#include <vector>
#include <optional>
#include "utils/Math.hpp"

// =============================================================================
// Level data types parsed from JSON
// =============================================================================

struct TileData {
    int  x = 0;
    int  y = 0;
    int  id = 0;
    bool solid = false;
    bool destructible = false;
};

struct EnemySpawnData {
    std::string type;       ///< "goomba", "koopa", "piranha_plant", "bowser"
    float x = 0.0f;
    float y = 0.0f;
    float patrolLeft  = 0.0f;
    float patrolRight = 0.0f;
    int   facing = 1;       ///< 1 = right, -1 = left
};

struct CollectibleSpawnData {
    std::string type;       ///< "coin", "mushroom", "fire_flower", "star", "1up"
    float x = 0.0f;
    float y = 0.0f;
};

struct QuestionBlockSpawnData {
    float x = 0.0f;
    float y = 0.0f;
    std::string contents;   ///< "coin", "mushroom", "fire_flower", "star", "1up"
};

struct PipeSpawnData {
    float x = 0.0f;
    float y = 0.0f;
    bool  enterable = false;
    float destX = 0.0f;
    float destY = 0.0f;
};

/// @brief Complete level description parsed from JSON.
struct LevelData {
    std::string name;
    std::string theme;
    int   widthTiles  = 0;
    int   heightTiles = 0;
    std::string music;
    float gravity   = 0.0f;   ///< 0 = use default Config::GRAVITY
    float timeLimit = 300.0f;
    std::vector<std::string> backgroundLayers; ///< Filenames for parallax layers
    std::vector<TileData> tiles;
    Vec2f playerSpawn;
    std::vector<EnemySpawnData>          enemies;
    std::vector<CollectibleSpawnData>    collectibles;
    std::vector<QuestionBlockSpawnData>  questionBlocks;
    std::vector<PipeSpawnData>           pipes;
    Vec2f goalPosition;
    std::string goalType = "flagpole";
    float flagPoleHeight = 288.0f;
};

// =============================================================================
// Loader
// =============================================================================

/// @brief Safe JSON level loader.
///        Reads via SafeFileIO, validates all required fields, returns nullopt
///        on any malformed input. Never crashes on bad JSON.
class LevelLoader {
public:
    /// @brief Load a level from a JSON file (path relative to SafeIO root).
    /// @return Parsed LevelData on success, std::nullopt on any error.
    [[nodiscard]] static std::optional<LevelData> load(const std::string& relativePath);
};
