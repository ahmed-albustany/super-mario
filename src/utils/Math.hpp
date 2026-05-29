#pragma once

#include <cmath>
#include <random>
#include <algorithm>
#include <cstdint>

/// 2D float vector with full operator set
struct Vec2f {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2f() = default;
    constexpr Vec2f(float x, float y) : x(x), y(y) {}

    constexpr Vec2f operator+(const Vec2f& o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2f operator-(const Vec2f& o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2f operator*(float s) const { return {x * s, y * s}; }
    constexpr Vec2f operator/(float s) const { return {x / s, y / s}; }
    constexpr Vec2f operator-() const { return {-x, -y}; }

    Vec2f& operator+=(const Vec2f& o) { x += o.x; y += o.y; return *this; }
    Vec2f& operator-=(const Vec2f& o) { x -= o.x; y -= o.y; return *this; }
    Vec2f& operator*=(float s) { x *= s; y *= s; return *this; }
    Vec2f& operator/=(float s) { x /= s; y /= s; return *this; }

    constexpr bool operator==(const Vec2f& o) const { return x == o.x && y == o.y; }
    constexpr bool operator!=(const Vec2f& o) const { return !(*this == o); }

    [[nodiscard]] float dot(const Vec2f& o) const { return x * o.x + y * o.y; }
    [[nodiscard]] float length() const { return std::sqrt(x * x + y * y); }
    [[nodiscard]] float lengthSq() const { return x * x + y * y; }

    [[nodiscard]] Vec2f normalized() const {
        float len = length();
        if (len > 0.0f) return *this / len;
        return {0.0f, 0.0f};
    }

    static Vec2f lerp(const Vec2f& a, const Vec2f& b, float t) {
        return a + (b - a) * t;
    }
};

inline Vec2f operator*(float s, const Vec2f& v) { return {v.x * s, v.y * s}; }

/// 2D integer vector
struct Vec2i {
    int x = 0;
    int y = 0;

    constexpr Vec2i() = default;
    constexpr Vec2i(int x, int y) : x(x), y(y) {}

    constexpr Vec2i operator+(const Vec2i& o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2i operator-(const Vec2i& o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2i operator*(int s) const { return {x * s, y * s}; }

    Vec2i& operator+=(const Vec2i& o) { x += o.x; y += o.y; return *this; }
    Vec2i& operator-=(const Vec2i& o) { x -= o.x; y -= o.y; return *this; }

    constexpr bool operator==(const Vec2i& o) const { return x == o.x && y == o.y; }
    constexpr bool operator!=(const Vec2i& o) const { return !(*this == o); }

    [[nodiscard]] Vec2f toFloat() const {
        return {static_cast<float>(x), static_cast<float>(y)};
    }

    static Vec2i fromFloat(const Vec2f& v) {
        return {static_cast<int>(v.x), static_cast<int>(v.y)};
    }
};

/// Axis-aligned rectangle
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    constexpr Rect() = default;
    constexpr Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}

    [[nodiscard]] constexpr float left()   const { return x; }
    [[nodiscard]] constexpr float right()  const { return x + w; }
    [[nodiscard]] constexpr float top()    const { return y; }
    [[nodiscard]] constexpr float bottom() const { return y + h; }
    [[nodiscard]] constexpr Vec2f center() const { return {x + w * 0.5f, y + h * 0.5f}; }

    [[nodiscard]] constexpr bool contains(const Vec2f& p) const {
        return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h;
    }

    [[nodiscard]] constexpr bool intersects(const Rect& o) const {
        return x < o.x + o.w && x + w > o.x &&
               y < o.y + o.h && y + h > o.y;
    }

    /// @brief Returns the overlap vector (penetration depth) between two rects.
    ///        Returns {0,0} if no intersection.
    [[nodiscard]] Vec2f overlap(const Rect& o) const {
        if (!intersects(o)) return {0.0f, 0.0f};

        float overlapX = std::min(right(), o.right()) - std::max(left(), o.left());
        float overlapY = std::min(bottom(), o.bottom()) - std::max(top(), o.top());

        float signX = (center().x < o.center().x) ? -1.0f : 1.0f;
        float signY = (center().y < o.center().y) ? -1.0f : 1.0f;

        return {overlapX * signX, overlapY * signY};
    }
};

namespace Math {

inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

inline float clamp(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

inline int clampInt(int v, int lo, int hi) {
    return std::max(lo, std::min(hi, v));
}

/// @brief Moves `current` toward `target` by at most `delta` per call.
///        Useful for smooth acceleration/deceleration.
inline float approach(float current, float target, float delta) {
    if (current < target) {
        return std::min(current + delta, target);
    }
    return std::max(current - delta, target);
}

/// @brief Thread-local Mersenne Twister RNG engine.
inline std::mt19937& rng() {
    static thread_local std::mt19937 gen(std::random_device{}());
    return gen;
}

inline int randInt(int minVal, int maxVal) {
    std::uniform_int_distribution<int> dist(minVal, maxVal);
    return dist(rng());
}

inline float randFloat(float minVal, float maxVal) {
    std::uniform_real_distribution<float> dist(minVal, maxVal);
    return dist(rng());
}

constexpr float PI = 3.14159265358979323846f;

inline float degToRad(float deg) {
    return deg * (PI / 180.0f);
}

inline float radToDeg(float rad) {
    return rad * (180.0f / PI);
}

/// @brief Safely add two integers, clamping to [min, max] instead of overflowing.
inline int safeAdd(int a, int b, int lo = 0, int hi = 999999999) {
    long long result = static_cast<long long>(a) + static_cast<long long>(b);
    return static_cast<int>(std::max(static_cast<long long>(lo),
                            std::min(static_cast<long long>(hi), result)));
}

} // namespace Math
