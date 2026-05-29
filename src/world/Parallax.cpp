#include "world/Parallax.hpp"
#include "core/GameConfig.hpp"

#include <cmath>

void Parallax::addLayer(TextureHandle texture, float texWidth, float texHeight,
                         float scrollSpeed) {
    m_layers.push_back({texture, texWidth, texHeight, scrollSpeed});
}

void Parallax::addDefaultLayers(TextureHandle far, float farW, float farH,
                                 TextureHandle mid, float midW, float midH,
                                 TextureHandle near, float nearW, float nearH) {
    m_layers.clear();
    addLayer(far,  farW,  farH,  SPEED_FAR);
    addLayer(mid,  midW,  midH,  SPEED_MID);
    addLayer(near, nearW, nearH, SPEED_NEAR);
}

void Parallax::clear() {
    m_layers.clear();
}

void Parallax::render(IPlatform& platform, const Vec2f& cameraOffset) const {
    float screenW = static_cast<float>(Config::WINDOW_WIDTH);
    float screenH = static_cast<float>(Config::WINDOW_HEIGHT);

    for (const auto& layer : m_layers) {
        if (!layer.texture.valid()) continue;
        if (layer.texWidth <= 0.0f) continue;

        // Parallax scroll: camera offset scaled by the layer's speed
        float scrollX = cameraOffset.x * layer.scrollSpeed;

        // Compute the starting draw X so the texture tiles seamlessly
        float startX = -std::fmod(scrollX, layer.texWidth);
        if (startX > 0.0f) startX -= layer.texWidth;

        // Vertical: position layer at the bottom of the screen if shorter than viewport,
        // otherwise pin to top
        float drawY = 0.0f;
        if (layer.texHeight < screenH) {
            drawY = screenH - layer.texHeight;
        }

        Rect srcRect = {0.0f, 0.0f, layer.texWidth, layer.texHeight};

        // Tile horizontally until we fill the screen
        for (float x = startX; x < screenW; x += layer.texWidth) {
            // Convert screen-space position to world-space so that the platform's
            // camera offset subtraction produces the correct screen position.
            Vec2f worldPos = {x + cameraOffset.x, drawY + cameraOffset.y};

            platform.drawSprite(layer.texture, srcRect, worldPos);
        }
    }
}
