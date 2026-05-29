#include <gtest/gtest.h>
#include "physics/AABB.hpp"
#include "utils/Math.hpp"

// =============================================================================
// Intersection tests
// =============================================================================

TEST(AABBIntersection, OverlappingRectsReturnTrue) {
    Rect a{0.0f, 0.0f, 50.0f, 50.0f};
    Rect b{25.0f, 25.0f, 50.0f, 50.0f};
    EXPECT_TRUE(AABB::intersects(a, b));
    EXPECT_TRUE(AABB::intersects(b, a));
}

TEST(AABBIntersection, NonOverlappingRectsReturnFalse) {
    Rect a{0.0f, 0.0f, 50.0f, 50.0f};
    Rect b{100.0f, 100.0f, 50.0f, 50.0f};
    EXPECT_FALSE(AABB::intersects(a, b));
    EXPECT_FALSE(AABB::intersects(b, a));
}

TEST(AABBIntersection, EdgeTouchingReturnsFalse) {
    // Right edge of a touches left edge of b (shared boundary, no overlap)
    Rect a{0.0f, 0.0f, 50.0f, 50.0f};
    Rect b{50.0f, 0.0f, 50.0f, 50.0f};
    EXPECT_FALSE(AABB::intersects(a, b));

    // Bottom edge of a touches top edge of b
    Rect c{0.0f, 0.0f, 50.0f, 50.0f};
    Rect d{0.0f, 50.0f, 50.0f, 50.0f};
    EXPECT_FALSE(AABB::intersects(c, d));
}

TEST(AABBIntersection, FullyContainedReturnsTrue) {
    Rect outer{0.0f, 0.0f, 100.0f, 100.0f};
    Rect inner{25.0f, 25.0f, 10.0f, 10.0f};
    EXPECT_TRUE(AABB::intersects(outer, inner));
    EXPECT_TRUE(AABB::intersects(inner, outer));
}

TEST(AABBIntersection, IdenticalRectsReturnTrue) {
    Rect a{10.0f, 10.0f, 40.0f, 40.0f};
    EXPECT_TRUE(AABB::intersects(a, a));
}

// =============================================================================
// Contains (point-in-rect)
// =============================================================================

TEST(AABBContains, PointInsideReturnsTrue) {
    Rect r{10.0f, 10.0f, 50.0f, 50.0f};
    EXPECT_TRUE(AABB::contains(r, {30.0f, 30.0f}));
}

TEST(AABBContains, PointOutsideReturnsFalse) {
    Rect r{10.0f, 10.0f, 50.0f, 50.0f};
    EXPECT_FALSE(AABB::contains(r, {5.0f, 5.0f}));
    EXPECT_FALSE(AABB::contains(r, {100.0f, 100.0f}));
}

// =============================================================================
// Penetration vector
// =============================================================================

TEST(AABBPenetration, NoOverlapReturnsZero) {
    Rect a{0.0f, 0.0f, 50.0f, 50.0f};
    Rect b{100.0f, 100.0f, 50.0f, 50.0f};
    Vec2f pen = AABB::penetration(a, b);
    EXPECT_FLOAT_EQ(pen.x, 0.0f);
    EXPECT_FLOAT_EQ(pen.y, 0.0f);
}

TEST(AABBPenetration, HorizontalOverlapCorrectDirection) {
    // a overlaps b from the left — smallest push is horizontal (push a left)
    Rect a{45.0f, 0.0f, 20.0f, 50.0f};
    Rect b{50.0f, 0.0f, 50.0f, 50.0f};
    Vec2f pen = AABB::penetration(a, b);

    // Push should be along X (horizontal overlap is smaller than vertical)
    EXPECT_NE(pen.x, 0.0f);
    EXPECT_FLOAT_EQ(pen.y, 0.0f);
}

TEST(AABBPenetration, VerticalOverlapCorrectDirection) {
    // a overlaps b from the top — smallest push is vertical
    Rect a{0.0f, 45.0f, 50.0f, 20.0f};
    Rect b{0.0f, 50.0f, 50.0f, 50.0f};
    Vec2f pen = AABB::penetration(a, b);

    // Push should be along Y
    EXPECT_FLOAT_EQ(pen.x, 0.0f);
    EXPECT_NE(pen.y, 0.0f);
}

TEST(AABBPenetration, MinimumTranslationVectorIsSmallest) {
    // Large vertical overlap but small horizontal overlap
    Rect a{48.0f, 10.0f, 10.0f, 30.0f};
    Rect b{50.0f, 0.0f,  50.0f, 50.0f};
    Vec2f pen = AABB::penetration(a, b);

    // Horizontal overlap (48+10 - 50 = 8) < vertical overlap
    // So penetration should be on X axis
    EXPECT_NE(pen.x, 0.0f);
    EXPECT_FLOAT_EQ(pen.y, 0.0f);
}

// =============================================================================
// Sweep tests
// =============================================================================

TEST(AABBSweep, MovingRectStopsAtBoundary) {
    // a moves right toward b
    Rect a{0.0f, 0.0f, 32.0f, 32.0f};
    Vec2f velocity{100.0f, 0.0f};
    Rect b{64.0f, 0.0f, 32.0f, 32.0f};

    auto result = AABB::sweep(a, velocity, b);

    EXPECT_TRUE(result.hit);
    // Distance to close = 64 - 32 = 32 pixels. velocity = 100. time = 32/100 = 0.32
    EXPECT_NEAR(result.time, 0.32f, 0.01f);
    // Normal should point left (opposing movement direction)
    EXPECT_FLOAT_EQ(result.normal.x, -1.0f);
    EXPECT_FLOAT_EQ(result.normal.y, 0.0f);
}

TEST(AABBSweep, MovingAwayNoFalsePositive) {
    // a moves left, away from b which is to the right
    Rect a{0.0f, 0.0f, 32.0f, 32.0f};
    Vec2f velocity{-100.0f, 0.0f};
    Rect b{64.0f, 0.0f, 32.0f, 32.0f};

    auto result = AABB::sweep(a, velocity, b);

    EXPECT_FALSE(result.hit);
}

TEST(AABBSweep, ZeroVelocityNoOverlapNoHit) {
    Rect a{0.0f, 0.0f, 32.0f, 32.0f};
    Vec2f velocity{0.0f, 0.0f};
    Rect b{64.0f, 0.0f, 32.0f, 32.0f};

    auto result = AABB::sweep(a, velocity, b);

    EXPECT_FALSE(result.hit);
}

TEST(AABBSweep, ZeroVelocityOverlappingHits) {
    Rect a{10.0f, 10.0f, 32.0f, 32.0f};
    Vec2f velocity{0.0f, 0.0f};
    Rect b{20.0f, 20.0f, 32.0f, 32.0f};

    auto result = AABB::sweep(a, velocity, b);

    EXPECT_TRUE(result.hit);
}

TEST(AABBSweep, VerticalSweepStopsCorrectly) {
    // a falls downward toward b
    Rect a{0.0f, 0.0f, 32.0f, 32.0f};
    Vec2f velocity{0.0f, 200.0f};
    Rect b{0.0f, 100.0f, 32.0f, 32.0f};

    auto result = AABB::sweep(a, velocity, b);

    EXPECT_TRUE(result.hit);
    // Distance = 100 - 32 = 68 pixels. velocity.y = 200. time = 68/200 = 0.34
    EXPECT_NEAR(result.time, 0.34f, 0.01f);
    EXPECT_FLOAT_EQ(result.normal.x, 0.0f);
    EXPECT_FLOAT_EQ(result.normal.y, -1.0f);
}

TEST(AABBSweep, MissParallelNoHit) {
    // a moves right but is above b entirely
    Rect a{0.0f, 0.0f, 32.0f, 32.0f};
    Vec2f velocity{100.0f, 0.0f};
    Rect b{64.0f, 64.0f, 32.0f, 32.0f};

    auto result = AABB::sweep(a, velocity, b);

    EXPECT_FALSE(result.hit);
}

// =============================================================================
// Minkowski expand
// =============================================================================

TEST(AABBMinkowski, ExpandCorrectDimensions) {
    Rect a{10.0f, 10.0f, 40.0f, 40.0f};
    Rect b{0.0f, 0.0f, 20.0f, 20.0f};

    Rect expanded = AABB::minkowskiExpand(a, b);

    EXPECT_FLOAT_EQ(expanded.x, 0.0f);   // 10 - 20*0.5
    EXPECT_FLOAT_EQ(expanded.y, 0.0f);   // 10 - 20*0.5
    EXPECT_FLOAT_EQ(expanded.w, 60.0f);  // 40 + 20
    EXPECT_FLOAT_EQ(expanded.h, 60.0f);  // 40 + 20
}
