#include "world/TileMap.hpp"
#include "world/LevelLoader.hpp"
#include "ecs/Components.hpp"
#include "utils/Logger.hpp"

#include <cmath>
#include <algorithm>

// =============================================================================
// Load
// =============================================================================

void TileMap::load(const std::vector<TileData>& tiles,
                   int widthTiles, int heightTiles,
                   TextureHandle tileset, int tilesetColumns,
                   entt::registry& reg) {
    m_width  = widthTiles;
    m_height = heightTiles;
    m_tileset = tileset;
    m_tilesetColumns = (tilesetColumns > 0) ? tilesetColumns : 16;

    // Allocate grid — default all air
    m_tiles.clear();
    m_tiles.resize(static_cast<size_t>(m_width) * static_cast<size_t>(m_height));

    // Populate from tile data
    for (const auto& td : tiles) {
        if (!inBounds(td.x, td.y)) continue;

        auto& tile = m_tiles[index(td.x, td.y)];
        tile.id           = td.id;
        tile.solid        = td.solid;
        tile.destructible = td.destructible;
        tile.destroyed    = false;

        // Create ECS collider entity for solid tiles
        if (tile.solid) {
            auto entity = reg.create();

            Vec2f worldPos = {
                static_cast<float>(td.x * Config::TILE_SIZE),
                static_cast<float>(td.y * Config::TILE_SIZE)
            };

            reg.emplace<TransformComponent>(entity, TransformComponent{worldPos});
            reg.emplace<ColliderComponent>(entity, ColliderComponent{
                {0.0f, 0.0f},
                {static_cast<float>(Config::TILE_SIZE), static_cast<float>(Config::TILE_SIZE)},
                false,  // isTrigger
                true    // isStatic
            });

            if (tile.destructible) {
                reg.emplace<DestructibleComponent>(entity, DestructibleComponent{1, false});
            }

            tile.entity = entity;
        }
    }

    LOG_INFO("TileMap loaded: " << m_width << "x" << m_height
             << " (" << tiles.size() << " tiles placed)");
}

// =============================================================================
// Render — only visible tiles, row-major order
// =============================================================================

void TileMap::render(IPlatform& platform, const Vec2f& cameraOffset) const {
    if (!m_tileset.valid()) return;

    float ts = static_cast<float>(Config::TILE_SIZE);

    // Visible tile range (with 1-tile margin for partially visible tiles)
    int startX = std::max(0, static_cast<int>(std::floor(cameraOffset.x / ts)));
    int startY = std::max(0, static_cast<int>(std::floor(cameraOffset.y / ts)));
    int endX   = std::min(m_width,
                   static_cast<int>(std::ceil((cameraOffset.x + Config::WINDOW_WIDTH) / ts)) + 1);
    int endY   = std::min(m_height,
                   static_cast<int>(std::ceil((cameraOffset.y + Config::WINDOW_HEIGHT) / ts)) + 1);

    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            const auto& tile = m_tiles[index(x, y)];
            if (tile.id == 0 || tile.destroyed) continue;

            // Source rect from tileset grid (tile IDs are 1-based)
            int col = (tile.id - 1) % m_tilesetColumns;
            int row = (tile.id - 1) / m_tilesetColumns;

            Rect srcRect = {
                static_cast<float>(col) * ts,
                static_cast<float>(row) * ts,
                ts, ts
            };

            Vec2f worldPos = {
                static_cast<float>(x) * ts,
                static_cast<float>(y) * ts
            };

            platform.drawSprite(m_tileset, srcRect, worldPos);
        }
    }
}

// =============================================================================
// Queries
// =============================================================================

const Tile& TileMap::getTile(int x, int y) const {
    if (!inBounds(x, y)) return m_emptyTile;
    return m_tiles[index(x, y)];
}

bool TileMap::isSolid(int x, int y) const {
    if (!inBounds(x, y)) return false;
    const auto& t = m_tiles[index(x, y)];
    return t.solid && !t.destroyed;
}

bool TileMap::isDestructible(int x, int y) const {
    if (!inBounds(x, y)) return false;
    const auto& t = m_tiles[index(x, y)];
    return t.destructible && !t.destroyed;
}

bool TileMap::inBounds(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

// =============================================================================
// Destroy tile (dash-through mechanic)
// =============================================================================

void TileMap::destroyTile(int x, int y, entt::registry& reg) {
    if (!inBounds(x, y)) return;

    auto& tile = m_tiles[index(x, y)];
    if (!tile.destructible || tile.destroyed) return;

    tile.destroyed = true;
    tile.solid     = false;

    // Remove the ECS collider entity
    if (tile.entity != entt::null && reg.valid(tile.entity)) {
        reg.destroy(tile.entity);
        tile.entity = entt::null;
    }

    LOG_DEBUG("Tile destroyed at (" << x << ", " << y << ")");
}
