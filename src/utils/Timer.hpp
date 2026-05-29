#pragma once

/// @brief Countdown timer — start with a duration, update each frame, check if finished.
class Timer {
public:
    Timer() = default;

    void start(float duration) {
        m_duration = duration;
        m_elapsed = 0.0f;
        m_running = true;
    }

    void update(float dt) {
        if (!m_running) return;
        m_elapsed += dt;
        if (m_elapsed >= m_duration) {
            m_elapsed = m_duration;
            m_running = false;
        }
    }

    [[nodiscard]] bool isFinished() const {
        return !m_running && m_elapsed >= m_duration && m_duration > 0.0f;
    }

    [[nodiscard]] bool isRunning() const { return m_running; }

    [[nodiscard]] float getRemainingTime() const {
        if (m_duration <= 0.0f) return 0.0f;
        float remaining = m_duration - m_elapsed;
        return remaining > 0.0f ? remaining : 0.0f;
    }

    [[nodiscard]] float getElapsedTime() const { return m_elapsed; }

    /// @brief Returns progress from 0.0 (just started) to 1.0 (finished).
    [[nodiscard]] float getNormalizedProgress() const {
        if (m_duration <= 0.0f) return 1.0f;
        float progress = m_elapsed / m_duration;
        return progress > 1.0f ? 1.0f : progress;
    }

    void reset() {
        m_elapsed = 0.0f;
        m_running = false;
        m_duration = 0.0f;
    }

private:
    float m_duration = 0.0f;
    float m_elapsed  = 0.0f;
    bool  m_running  = false;
};

/// @brief Stopwatch — counts up indefinitely until stopped.
class Stopwatch {
public:
    Stopwatch() = default;

    void start() {
        m_running = true;
    }

    void stop() {
        m_running = false;
    }

    void update(float dt) {
        if (m_running) {
            m_elapsed += dt;
        }
    }

    void reset() {
        m_elapsed = 0.0f;
        m_running = false;
    }

    [[nodiscard]] float getElapsed() const { return m_elapsed; }
    [[nodiscard]] bool isRunning() const { return m_running; }

private:
    float m_elapsed = 0.0f;
    bool  m_running = false;
};
