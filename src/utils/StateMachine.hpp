#pragma once

#include <set>
#include <vector>
#include <functional>
#include <utility>
#include "utils/Logger.hpp"

/// @brief Generic finite state machine with whitelisted transitions.
///        Every valid transition must be registered via addTransition().
///        Invalid transitions are rejected and logged, never crash.
/// @tparam StateEnum An enum or enum class identifying states.
template<typename StateEnum>
class StateMachine {
public:
    using TransitionFn = std::function<void(StateEnum from, StateEnum to)>;

    explicit StateMachine(StateEnum initial)
        : m_current(initial)
        , m_previous(initial)
    {}

    /// @brief Register a valid transition edge.
    void addTransition(StateEnum from, StateEnum to) {
        m_validTransitions.insert({from, to});
    }

    /// @brief Register multiple transitions from one state to several targets.
    void addTransitions(StateEnum from, std::initializer_list<StateEnum> targets) {
        for (auto to : targets) {
            m_validTransitions.insert({from, to});
        }
    }

    /// @brief Attempt to transition to newState.
    /// @return true if transition succeeded, false if invalid or already in that state.
    bool transition(StateEnum newState) {
        if (newState == m_current) {
            return true; // already there
        }

        auto edge = std::make_pair(m_current, newState);
        if (m_validTransitions.find(edge) == m_validTransitions.end()) {
            LOG_WARN("StateMachine: rejected invalid transition from "
                     << static_cast<int>(m_current) << " to "
                     << static_cast<int>(newState));
            return false;
        }

        m_previous = m_current;
        m_current = newState;
        m_stateTime = 0.0f;

        for (auto& fn : m_callbacks) {
            fn(m_previous, m_current);
        }

        return true;
    }

    /// @brief Force a state change without checking the transition whitelist.
    ///        Use only for initialization or error recovery.
    void forceState(StateEnum state) {
        m_previous = m_current;
        m_current = state;
        m_stateTime = 0.0f;
    }

    [[nodiscard]] StateEnum current() const { return m_current; }
    [[nodiscard]] StateEnum previous() const { return m_previous; }
    [[nodiscard]] bool isIn(StateEnum s) const { return m_current == s; }

    /// @brief Register a callback fired on every successful transition.
    void onTransition(TransitionFn fn) {
        m_callbacks.push_back(std::move(fn));
    }

    /// @brief Seconds spent in the current state. Updated by tick().
    [[nodiscard]] float timeInCurrentState() const { return m_stateTime; }

    /// @brief Call once per frame to advance the state timer.
    void tick(float dt) {
        m_stateTime += dt;
    }

private:
    StateEnum m_current;
    StateEnum m_previous;
    float m_stateTime = 0.0f;
    std::set<std::pair<StateEnum, StateEnum>> m_validTransitions;
    std::vector<TransitionFn> m_callbacks;
};
