#include "world/Camera.hpp"

Camera::Camera()
    : m_position{Config::WINDOW_WIDTH * 0.5f, Config::WINDOW_HEIGHT * 0.5f}
    , m_target{m_position}
    , m_levelWidth{static_cast<float>(Config::LEVEL_WIDTH_TILES * Config::TILE_SIZE)}
    , m_levelHeight{static_cast<float>(Config::LEVEL_HEIGHT_TILES * Config::TILE_SIZE)}
{}

void Camera::setTarget(const Vec2f& target) {
    m_target = target;
}

void Camera::setBounds(float levelWidth, float levelHeight) {
    m_levelWidth  = levelWidth;
    m_levelHeight = levelHeight;
}

void Camera::update(float dt) {
    // Smooth lerp toward target
    float t = 1.0f - std::exp(-Config::CAMERA_LERP * dt);
    m_position = Vec2f::lerp(m_position, m_target, t);

    // Hard clamp to level bounds so camera never shows outside the level
    float halfW = static_cast<float>(Config::WINDOW_WIDTH)  * 0.5f;
    float halfH = static_cast<float>(Config::WINDOW_HEIGHT) * 0.5f;

    if (m_levelWidth > 0.0f) {
        m_position.x = Math::clamp(m_position.x, halfW, m_levelWidth - halfW);
    }
    if (m_levelHeight > 0.0f) {
        m_position.y = Math::clamp(m_position.y, halfH, m_levelHeight - halfH);
    }

    // Decay screen shake
    if (m_shakeIntensity > 0.0f) {
        m_shakeIntensity = Math::approach(m_shakeIntensity, 0.0f,
                                           Config::SCREEN_SHAKE_DECAY * dt);
        if (m_shakeIntensity < 0.05f) {
            m_shakeIntensity = 0.0f;
        }
    }
}

void Camera::addShake(float intensity) {
    m_shakeIntensity += intensity;
    if (m_shakeIntensity > MAX_SHAKE) {
        m_shakeIntensity = MAX_SHAKE;
    }
}

Vec2f Camera::getViewOffset() const {
    float halfW = static_cast<float>(Config::WINDOW_WIDTH)  * 0.5f;
    float halfH = static_cast<float>(Config::WINDOW_HEIGHT) * 0.5f;

    Vec2f offset = {m_position.x - halfW, m_position.y - halfH};

    // Apply screen shake as random offset
    if (m_shakeIntensity > 0.1f) {
        offset.x += Math::randFloat(-m_shakeIntensity, m_shakeIntensity);
        offset.y += Math::randFloat(-m_shakeIntensity, m_shakeIntensity);
    }

    return offset;
}
