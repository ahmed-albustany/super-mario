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
    std::string type;
    float x = 0.0f;
    float y = 0.0f;
    float patrolLeft  = 0.0f;
    float patrolRight = 0.0f;
    int   facing = 1;
};

struct CollectibleSpawnData {
    std::string type;
    float x = 0.0f;
    float y = 0.0f;
};

struct QuestionBlockSpawnData {
    float x = 0.0f;
    float y = 0.0f;
    std::string contents;
};

struct PipeSpawnData {
    float x = 0.0f;
    float y = 0.0f;
    bool  enterable = false;
    float destX = 0.0f;
    float destY = 0.0f;
};

/// @brief Fruit spawn data for Pixel Adventure.
struct FruitSpawnData {
    std::string type;  ///< "cherry", "apple", "orange", etc.
    float x = 0.0f;
    float y = 0.0f;
};

/// @brief Trap spawn data for all trap types.
struct TrapSpawnData {
    std::string type;  ///< "saw", "spike_head", "rock_head", "fire", "arrow",
                       ///< "falling_platform", "moving_platform", "fan",
                       ///< "spiked_ball", "spikes", "trampoline"
    float x = 0.0f;
    float y = 0.0f;
    float speed = 1.0f;
    float onTime  = 2.0f;
    float offTime = 1.0f;
    float strength = 400.0f;       ///< Fan strength
    float chainLength = 8.0f;      ///< Spiked ball chain
    std::string direction = "right"; ///< Arrow direction
    std::vector<Vec2f> path;       ///< Moving platform path
};

/// @brief Box spawn data.
struct BoxSpawnData {
    std::string type;  ///< "box1", "box2", "box3"
    float x = 0.0f;
    float y = 0.0f;
    int   hits = 3;
};

/// @brief Checkpoint spawn data.
struct CheckpointSpawnData {
    float x = 0.0f;
    float y = 0.0f;
};

/// @brief Complete level description parsed from JSON.
struct LevelData {
    std::string name;
    std::string theme;
    int   widthTiles  = 0;
    int   heightTiles = 0;
    std::string music;
    float gravity   = 0.0f;
    float timeLimit = 300.0f;
    std::string background;  ///< Background texture key
    std::vector<std::string> backgroundLayers;
    std::vector<TileData> tiles;
    Vec2f playerSpawn;

    // Legacy spawn data
    std::vector<EnemySpawnData>          enemies;
    std::vector<CollectibleSpawnData>    collectibles;
    std::vector<QuestionBlockSpawnData>  questionBlocks;
    std::vector<PipeSpawnData>           pipes;

    // Pixel Adventure spawn data
    std::vector<FruitSpawnData>      fruits;
    std::vector<TrapSpawnData>       traps;
    std::vector<BoxSpawnData>        boxes;
    std::vector<CheckpointSpawnData> checkpoints;

    Vec2f goalPosition;
    std::string goalType = "trophy";
    float flagPoleHeight = 288.0f;
};

// =============================================================================
// Loader
// =============================================================================

class LevelLoader {
public:
    [[nodiscard]] static std::optional<LevelData> load(const std::string& relativePath);
};
