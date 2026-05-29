#pragma once

#include "utils/Math.hpp"
#include "core/GameConfig.hpp"

/// @brief Smooth-follow camera with hard level bounds and screen shake.
///        Call setTarget() to follow the player, update() each frame,
///        and pass getViewOffset() to the platform's setCameraOffset().
class Camera {
public:
    Camera();

    /// @brief Set the point the camera should follow (usually player center).
    void setTarget(const Vec2f& target);

    /// @brief Set the hard bounds (level pixel dimensions).
    void setBounds(float levelWidth, float levelHeight);

    /// @brief Advance camera: lerp toward target, clamp to bounds, decay shake.
    void update(float dt);

    /// @brief Add screen shake. Intensities stack (capped internally).
    void addShake(float intensity);

    /// @brief Final view offset (top-left corner of the camera in world space).
    ///        Includes shake. Pass this to platform.setCameraOffset().
    [[nodiscard]] Vec2f getViewOffset() const;

    /// @brief Raw camera position (center of viewport, no shake).
    [[nodiscard]] Vec2f getPosition() const { return m_position; }

private:
    Vec2f m_position;       ///< Camera center in world space
    Vec2f m_target;         ///< Desired center
    float m_shakeIntensity = 0.0f;
    float m_levelWidth  = 0.0f;
    float m_levelHeight = 0.0f;

    static constexpr float MAX_SHAKE = 12.0f;
};
