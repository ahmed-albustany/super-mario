#pragma once

#include <vector>
#include <string>
#include <entt/entt.hpp>
#include "utils/Math.hpp"
#include "platform/IPlatform.hpp"
#include "core/GameConfig.hpp"

/// @brief Internal tile data stored in the flat grid.
struct Tile {
    int  id           = 0;     ///< 0 = air/empty, 1+ = tile type
    bool solid        = false;
    bool destructible = false;
    bool destroyed    = false;
    entt::entity entity = entt::null; ///< Corresponding ECS entity (for solid tiles)
};

/// @brief 2D tile grid loaded from level data.
///        Stores tiles as a flat row-major vector, batch-renders visible tiles,
///        and creates ECS collider entities for solid tiles.
class TileMap {
public:
    TileMap() = default;

    /// @brief Build the tile grid from raw tile data and create ECS collider entities.
    /// @param tiles       Raw tile entries from level JSON.
    /// @param widthTiles  Level width in tiles.
    /// @param heightTiles Level height in tiles.
    /// @param tilesetKey  ResourceManager key for the tileset texture.
    /// @param reg         ECS registry to populate with tile collider entities.
    /// @param platform    Platform for resolving texture handle.
    void load(const std::vector<struct TileData>& tiles,
              int widthTiles, int heightTiles,
              TextureHandle tileset, int tilesetColumns,
              entt::registry& reg);

    /// @brief Render only the tiles visible within the current camera view.
    void render(IPlatform& platform, const Vec2f& cameraOffset) const;

    // ---- Tile queries ----

    [[nodiscard]] const Tile& getTile(int x, int y) const;
    [[nodiscard]] bool isSolid(int x, int y) const;
    [[nodiscard]] bool isDestructible(int x, int y) const;
    [[nodiscard]] bool inBounds(int x, int y) const;

    /// @brief Destroy a destructible tile (dash-through mechanic).
    ///        Marks internal data as destroyed and removes the ECS collider entity.
    void destroyTile(int x, int y, entt::registry& reg);

    [[nodiscard]] int getWidth()  const { return m_width; }
    [[nodiscard]] int getHeight() const { return m_height; }

    /// @brief World-space pixel dimensions.
    [[nodiscard]] float getPixelWidth()  const { return static_cast<float>(m_width  * Config::TILE_SIZE); }
    [[nodiscard]] float getPixelHeight() const { return static_cast<float>(m_height * Config::TILE_SIZE); }

private:
    [[nodiscard]] size_t index(int x, int y) const {
        return static_cast<size_t>(y) * static_cast<size_t>(m_width) + static_cast<size_t>(x);
    }

    std::vector<Tile> m_tiles;
    int m_width  = 0;
    int m_height = 0;
    TextureHandle m_tileset;
    int m_tilesetColumns = 16; ///< Number of tile columns in the tileset image
    Tile m_emptyTile;          ///< Returned for out-of-bounds queries
};
