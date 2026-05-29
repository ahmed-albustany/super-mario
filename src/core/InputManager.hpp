#pragma once

#include <array>
#include <unordered_map>
#include <string>
#include "platform/IPlatform.hpp"

/// @brief High-level game actions, decoupled from raw key codes.
enum class Action {
    MoveLeft,
    MoveRight,
    Jump,
    Dash,
    Pause,
    Confirm,
    Back,
    COUNT
};

/// @brief Wraps IPlatform input with action mapping, input filtering,
///        and touch virtual button support.
///
///        Game code calls isHeld(Action) / isJustPressed(Action) — never
///        checks raw key codes directly.
class InputManager {
public:
    InputManager();

    /// @brief Bind the InputManager to a platform. Must be called before use.
    void init(IPlatform& platform);

    /// @brief Call once per frame AFTER platform->pollEvents().
    ///        Reads platform key state, merges touch input, and applies filters.
    void update();

    // ---- Action queries ----

    /// @brief Is the action currently held down?
    [[nodiscard]] bool isHeld(Action action) const;

    /// @brief Was the action just pressed this frame?
    [[nodiscard]] bool isJustPressed(Action action) const;

    /// @brief Was the action just released this frame?
    [[nodiscard]] bool isJustReleased(Action action) const;

    // ---- Touch virtual buttons (for WASM/mobile overlay) ----

    /// @brief Set a virtual touch button state. Called by the web touch overlay JS.
    void setTouchButtonState(Action action, bool pressed);

    // ---- Key rebinding ----

    /// @brief Rebind an action to a different key code.
    void bind(Action action, KeyCode key);

    /// @brief Get the current primary key bound to an action.
    [[nodiscard]] KeyCode getBinding(Action action) const;

    // ---- Mouse / touch position ----

    [[nodiscard]] Vec2f getPointerPosition() const;
    [[nodiscard]] bool isPointerDown() const;

private:
    static constexpr int ACTION_COUNT = static_cast<int>(Action::COUNT);

    void initDefaultBindings();

    IPlatform* m_platform = nullptr;

    // Current and previous frame action state (after filtering)
    std::array<bool, ACTION_COUNT> m_currState{};
    std::array<bool, ACTION_COUNT> m_prevState{};

    // Touch virtual button state
    std::array<bool, ACTION_COUNT> m_touchState{};

    // Action → KeyCode mapping
    std::array<KeyCode, ACTION_COUNT> m_bindings{};
};
