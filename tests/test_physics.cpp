#include <gtest/gtest.h>
#include <entt/entt.hpp>
#include "ecs/systems/PhysicsSystem.hpp"
#include "ecs/Components.hpp"
#include "core/GameConfig.hpp"

// =============================================================================
// Helpers
// =============================================================================

class PhysicsSystemTest : public ::testing::Test {
protected:
    entt::registry registry;
    PhysicsSystem system;

    /// Create a basic gravity-affected entity with optional initial velocity.
    entt::entity makeEntity(Vec2f vel = {0.0f, 0.0f}, float gravMult = 1.0f) {
        auto e = registry.create();
        registry.emplace<VelocityComponent>(e, VelocityComponent{vel});
        registry.emplace<GravityComponent>(e, GravityComponent{gravMult});
        return e;
    }

    /// Create a player entity in a specific state.
    entt::entity makePlayer(PlayerState state, Vec2f vel = {0.0f, 0.0f}) {
        auto e = makeEntity(vel);
        PlayerComponent pc{};
        pc.state = state;
        registry.emplace<PlayerComponent>(e, pc);
        return e;
    }
};

// =============================================================================
// Tests
// =============================================================================

TEST_F(PhysicsSystemTest, GravityAcceleratesVelocityOverTime) {
    auto e = makeEntity();
    float dt = 1.0f / 60.0f;

    system.update(registry, dt);

    auto& vel = registry.get<VelocityComponent>(e);
    float expected = Config::GRAVITY * dt;
    EXPECT_NEAR(vel.velocity.y, expected, 0.01f);

    // After a second tick, velocity should double
    system.update(registry, dt);
    EXPECT_NEAR(vel.velocity.y, expected * 2.0f, 0.01f);
}

TEST_F(PhysicsSystemTest, TerminalVelocityNeverExceeded) {
    // Start with velocity already near terminal
    auto e = makeEntity({0.0f, Config::TERMINAL_VELOCITY - 10.0f});

    // Large dt to push way past terminal
    system.update(registry, 1.0f);

    auto& vel = registry.get<VelocityComponent>(e);
    EXPECT_LE(vel.velocity.y, Config::TERMINAL_VELOCITY);
}

TEST_F(PhysicsSystemTest, TerminalVelocityUpwardCapped) {
    // Extreme upward velocity
    auto e = makeEntity({0.0f, -Config::TERMINAL_VELOCITY - 500.0f});

    system.update(registry, 1.0f / 60.0f);

    auto& vel = registry.get<VelocityComponent>(e);
    EXPECT_GE(vel.velocity.y, -Config::TERMINAL_VELOCITY);
}

TEST_F(PhysicsSystemTest, WallSlideReducesGravity) {
    auto e = makePlayer(PlayerState::WallSliding);
    float dt = 1.0f / 60.0f;

    system.update(registry, dt);

    auto& vel = registry.get<VelocityComponent>(e);
    float expected = Config::WALL_SLIDE_GRAVITY * dt;
    EXPECT_NEAR(vel.velocity.y, expected, 0.01f);

    // Confirm it's less than normal gravity would produce
    float normalGravityResult = Config::GRAVITY * dt;
    EXPECT_LT(vel.velocity.y, normalGravityResult);
}

TEST_F(PhysicsSystemTest, ZeroGravityMultiplierProducesNoChange) {
    auto e = makeEntity({0.0f, 0.0f}, 0.0f);

    system.update(registry, 1.0f / 60.0f);

    auto& vel = registry.get<VelocityComponent>(e);
    EXPECT_FLOAT_EQ(vel.velocity.y, 0.0f);
}

TEST_F(PhysicsSystemTest, DashingPlayerIgnoresGravity) {
    auto e = makePlayer(PlayerState::Dashing, {Config::PLAYER_DASH_SPEED, 0.0f});

    system.update(registry, 1.0f / 60.0f);

    auto& vel = registry.get<VelocityComponent>(e);
    EXPECT_FLOAT_EQ(vel.velocity.y, 0.0f);
}

TEST_F(PhysicsSystemTest, HorizontalVelocityCapped) {
    float maxHoriz = Config::PLAYER_DASH_SPEED * 1.5f;
    auto e = makeEntity({maxHoriz + 500.0f, 0.0f});

    system.update(registry, 1.0f / 60.0f);

    auto& vel = registry.get<VelocityComponent>(e);
    EXPECT_LE(vel.velocity.x, maxHoriz);
}

TEST_F(PhysicsSystemTest, ProjectileLifetimeDecrementsAndDestroysAtZero) {
    auto e = registry.create();
    registry.emplace<ProjectileComponent>(e, ProjectileComponent{0, 1, 0.5f, 300.0f, {1.0f, 0.0f}});

    // Tick 30 frames at 1/60s each = 0.5s total
    for (int i = 0; i < 30; ++i) {
        system.update(registry, 1.0f / 60.0f);
    }

    // Lifetime should be at or below zero, entity marked for destruction
    auto& proj = registry.get<ProjectileComponent>(e);
    EXPECT_LE(proj.lifetime, 0.0f);
    EXPECT_TRUE(registry.all_of<DestroyFlag>(e));
}

TEST_F(PhysicsSystemTest, MultipleEntitiesProcessedIndependently) {
    auto e1 = makeEntity({0.0f, 0.0f}, 1.0f);
    auto e2 = makeEntity({0.0f, 0.0f}, 0.5f);
    float dt = 1.0f / 60.0f;

    system.update(registry, dt);

    float v1 = registry.get<VelocityComponent>(e1).velocity.y;
    float v2 = registry.get<VelocityComponent>(e2).velocity.y;

    EXPECT_NEAR(v1, Config::GRAVITY * dt, 0.01f);
    EXPECT_NEAR(v2, Config::GRAVITY * 0.5f * dt, 0.01f);
    EXPECT_GT(v1, v2);
}
