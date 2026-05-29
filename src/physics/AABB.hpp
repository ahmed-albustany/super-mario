#pragma once

#include "utils/Math.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

/// @brief AABB collision math utilities used by CollisionSystem.
namespace AABB {

/// @brief Result of a swept AABB test.
struct SweptResult {
    float time   = 1.0f;           ///< 0–1 fraction of velocity before first contact
    Vec2f normal  = {0.0f, 0.0f};  ///< Surface normal at collision point
    bool  hit     = false;
};

/// @brief Static intersection test between two rectangles.
[[nodiscard]] inline bool intersects(const Rect& a, const Rect& b) {
    return a.left() < b.right()  && a.right()  > b.left() &&
           a.top()  < b.bottom() && a.bottom() > b.top();
}

/// @brief Check if a point is inside a rectangle.
[[nodiscard]] inline bool contains(const Rect& r, const Vec2f& p) {
    return p.x >= r.left() && p.x <= r.right() &&
           p.y >= r.top()  && p.y <= r.bottom();
}

/// @brief Calculate the minimum translation vector to separate two overlapping AABBs.
///        Returns {0,0} if no overlap. The sign indicates which direction to push `a`.
[[nodiscard]] inline Vec2f penetration(const Rect& a, const Rect& b) {
    if (!intersects(a, b)) return {0.0f, 0.0f};

    float left   = b.right()  - a.left();    // push a right
    float right  = a.right()  - b.left();    // push a left  (negate)
    float top    = b.bottom() - a.top();     // push a down
    float bottom = a.bottom() - b.top();     // push a up    (negate)

    // Choose the smallest absolute penetration axis
    float minX = (left < right) ? left : -right;
    float minY = (top < bottom) ? top  : -bottom;

    if (std::abs(minX) < std::abs(minY)) {
        return {minX, 0.0f};
    }
    return {0.0f, minY};
}

/// @brief Swept AABB: test a moving box `a` with `velocity` against a static box `b`.
///        Returns the fraction of velocity at which contact first occurs (0–1),
///        the collision normal, and whether a hit occurred within the velocity step.
[[nodiscard]] inline SweptResult sweep(const Rect& a, const Vec2f& velocity, const Rect& b) {
    SweptResult result;

    if (velocity.x == 0.0f && velocity.y == 0.0f) {
        result.hit = intersects(a, b);
        result.time = 0.0f;
        return result;
    }

    // Entry and exit distances per axis
    float xEntryDist, xExitDist;
    float yEntryDist, yExitDist;

    if (velocity.x > 0.0f) {
        xEntryDist = b.left()  - a.right();
        xExitDist  = b.right() - a.left();
    } else {
        xEntryDist = b.right() - a.left();
        xExitDist  = b.left()  - a.right();
    }

    if (velocity.y > 0.0f) {
        yEntryDist = b.top()    - a.bottom();
        yExitDist  = b.bottom() - a.top();
    } else {
        yEntryDist = b.bottom() - a.top();
        yExitDist  = b.top()    - a.bottom();
    }

    // Entry and exit times per axis
    float xEntry, xExit;
    float yEntry, yExit;

    constexpr float INF = std::numeric_limits<float>::infinity();

    if (velocity.x == 0.0f) {
        xEntry = -INF;
        xExit  =  INF;
    } else {
        xEntry = xEntryDist / velocity.x;
        xExit  = xExitDist  / velocity.x;
    }

    if (velocity.y == 0.0f) {
        yEntry = -INF;
        yExit  =  INF;
    } else {
        yEntry = yEntryDist / velocity.y;
        yExit  = yExitDist  / velocity.y;
    }

    float entryTime = std::max(xEntry, yEntry);
    float exitTime  = std::min(xExit,  yExit);

    // No collision conditions
    if (entryTime > exitTime)   return result;
    if (xEntry < 0.0f && yEntry < 0.0f) return result;
    if (xEntry > 1.0f || yEntry > 1.0f) return result;
    if (entryTime < 0.0f)       return result;

    result.time = entryTime;
    result.hit  = true;

    // Determine collision normal from the axis of last entry
    if (xEntry > yEntry) {
        result.normal = {(velocity.x < 0.0f) ? 1.0f : -1.0f, 0.0f};
    } else {
        result.normal = {0.0f, (velocity.y < 0.0f) ? 1.0f : -1.0f};
    }

    return result;
}

/// @brief Expand rect `a` by the half-size of rect `b` (Minkowski sum).
///        Useful for reducing a box-vs-box test to a point-vs-box test.
[[nodiscard]] inline Rect minkowskiExpand(const Rect& a, const Rect& b) {
    return {
        a.x - b.w * 0.5f,
        a.y - b.h * 0.5f,
        a.w + b.w,
        a.h + b.h
    };
}

} // namespace AABB
