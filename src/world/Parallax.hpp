#pragma once

#include <vector>
#include <string>
#include "platform/IPlatform.hpp"
#include "utils/Math.hpp"

/// @brief Multi-layer scrolling background.
///        Each layer scrolls at a different speed relative to the camera,
///        creating a depth illusion. Tiles infinitely in the horizontal axis.
///        Drawn before all entities (lowest z-order).
class Parallax {
public:
    /// @brief Default scroll speed multipliers for 3 standard layers.
    static constexpr float SPEED_FAR    = 0.1f;
    static constexpr float SPEED_MID    = 0.3f;
    static constexpr float SPEED_NEAR   = 0.6f;

    Parallax() = default;

    /// @brief Add a layer with a given scroll speed.
    /// @param texture   Handle to the layer texture (already loaded).
    /// @param texWidth  Width of one tile of the texture in pixels.
    /// @param texHeight Height of the texture in pixels.
    /// @param scrollSpeed  Fraction of camera movement (0.0 = static, 1.0 = full speed).
    void addLayer(TextureHandle texture, float texWidth, float texHeight,
                  float scrollSpeed);

    /// @brief Convenience: add the standard 3-layer set (far / mid / near).
    void addDefaultLayers(TextureHandle far, float farW, float farH,
                          TextureHandle mid, float midW, float midH,
                          TextureHandle near, float nearW, float nearH);

    /// @brief Clear all layers.
    void clear();

    /// @brief Render all layers (back-to-front) before entities.
    ///        Must be called with the current camera offset so parallax
    ///        scroll is relative to the viewport.
    void render(IPlatform& platform, const Vec2f& cameraOffset) const;

private:
    struct Layer {
        TextureHandle texture;
        float texWidth   = 0.0f;
        float texHeight  = 0.0f;
        float scrollSpeed = 0.0f;
    };

    std::vector<Layer> m_layers;
};
