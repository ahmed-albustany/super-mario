#include <gtest/gtest.h>
#include "utils/StateMachine.hpp"

// =============================================================================
// Test state enum
// =============================================================================

enum class TestState { A, B, C, D };

// =============================================================================
// Fixture
// =============================================================================

class StateMachineTest : public ::testing::Test {
protected:
    StateMachine<TestState> sm{TestState::A};

    void SetUp() override {
        sm.addTransition(TestState::A, TestState::B);
        sm.addTransition(TestState::B, TestState::C);
        sm.addTransition(TestState::C, TestState::A);
        sm.addTransition(TestState::A, TestState::C);
    }
};

// =============================================================================
// Tests
// =============================================================================

TEST_F(StateMachineTest, ValidTransitionSucceeds) {
    EXPECT_TRUE(sm.transition(TestState::B));
    EXPECT_EQ(sm.current(), TestState::B);
}

TEST_F(StateMachineTest, InvalidTransitionRejected) {
    // B -> A is not registered
    sm.transition(TestState::B);
    EXPECT_EQ(sm.current(), TestState::B);

    EXPECT_FALSE(sm.transition(TestState::A));
    EXPECT_EQ(sm.current(), TestState::B);
}

TEST_F(StateMachineTest, TransitionToCurrentStateSucceeds) {
    // Same-state transitions return true without firing callbacks
    EXPECT_TRUE(sm.transition(TestState::A));
    EXPECT_EQ(sm.current(), TestState::A);
}

TEST_F(StateMachineTest, PreviousReturnsCorrectPriorState) {
    // Initial: previous == current == A
    EXPECT_EQ(sm.previous(), TestState::A);

    sm.transition(TestState::B);
    EXPECT_EQ(sm.previous(), TestState::A);
    EXPECT_EQ(sm.current(), TestState::B);

    sm.transition(TestState::C);
    EXPECT_EQ(sm.previous(), TestState::B);
    EXPECT_EQ(sm.current(), TestState::C);
}

TEST_F(StateMachineTest, TimeInCurrentStateIncrementsWithTick) {
    EXPECT_FLOAT_EQ(sm.timeInCurrentState(), 0.0f);

    sm.tick(0.5f);
    EXPECT_NEAR(sm.timeInCurrentState(), 0.5f, 0.001f);

    sm.tick(0.25f);
    EXPECT_NEAR(sm.timeInCurrentState(), 0.75f, 0.001f);
}

TEST_F(StateMachineTest, TimeResetsOnTransition) {
    sm.tick(1.0f);
    EXPECT_NEAR(sm.timeInCurrentState(), 1.0f, 0.001f);

    sm.transition(TestState::B);
    EXPECT_FLOAT_EQ(sm.timeInCurrentState(), 0.0f);
}

TEST_F(StateMachineTest, TimeDoesNotResetOnRejectedTransition) {
    sm.tick(1.0f);

    // D is not reachable from A
    sm.transition(TestState::D);

    EXPECT_NEAR(sm.timeInCurrentState(), 1.0f, 0.001f);
}

TEST_F(StateMachineTest, CallbackFiresOncePerTransition) {
    int callCount = 0;
    TestState cbFrom = TestState::A;
    TestState cbTo = TestState::A;

    sm.onTransition([&](TestState from, TestState to) {
        ++callCount;
        cbFrom = from;
        cbTo = to;
    });

    sm.transition(TestState::B);
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(cbFrom, TestState::A);
    EXPECT_EQ(cbTo, TestState::B);

    sm.transition(TestState::C);
    EXPECT_EQ(callCount, 2);
    EXPECT_EQ(cbFrom, TestState::B);
    EXPECT_EQ(cbTo, TestState::C);
}

TEST_F(StateMachineTest, CallbackDoesNotFireOnRejectedTransition) {
    int callCount = 0;
    sm.onTransition([&](TestState, TestState) { ++callCount; });

    sm.transition(TestState::D);  // not registered
    EXPECT_EQ(callCount, 0);
}

TEST_F(StateMachineTest, CallbackDoesNotFireOnSameStateTransition) {
    int callCount = 0;
    sm.onTransition([&](TestState, TestState) { ++callCount; });

    sm.transition(TestState::A);  // already in A
    EXPECT_EQ(callCount, 0);
}

TEST_F(StateMachineTest, MultipleCallbacksAllFire) {
    int count1 = 0;
    int count2 = 0;

    sm.onTransition([&](TestState, TestState) { ++count1; });
    sm.onTransition([&](TestState, TestState) { ++count2; });

    sm.transition(TestState::B);
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
}

TEST_F(StateMachineTest, ForceStateBypassesWhitelist) {
    sm.forceState(TestState::D);
    EXPECT_EQ(sm.current(), TestState::D);
    EXPECT_EQ(sm.previous(), TestState::A);
    EXPECT_FLOAT_EQ(sm.timeInCurrentState(), 0.0f);
}

TEST_F(StateMachineTest, AddTransitionsFromInitializerList) {
    StateMachine<TestState> sm2{TestState::A};
    sm2.addTransitions(TestState::A, {TestState::B, TestState::C, TestState::D});

    EXPECT_TRUE(sm2.transition(TestState::B));
    sm2.forceState(TestState::A);
    EXPECT_TRUE(sm2.transition(TestState::C));
    sm2.forceState(TestState::A);
    EXPECT_TRUE(sm2.transition(TestState::D));
}
