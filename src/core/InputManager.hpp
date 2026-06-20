#pragma once

#include <array>
#include <string>
#include "platform/IPlatform.hpp"

/// @brief High-level game actions, decoupled from raw key codes.
enum class Action {
    MoveLeft,
    MoveRight,
    MoveDown,    ///< For entering pipes / crouching
    Jump,        ///< First jump
    DoubleJump,  ///< Second jump (separate key in PIXEL RUSH)
    Pause,
    Confirm,
    Back,
    COUNT
};

/// @brief Wraps IPlatform input with action mapping, input filtering,
///        and touch virtual button support.
///        Supports up to 4 players with independent key bindings.
class InputManager {
public:
    static constexpr int MAX_PLAYERS = 4;

    InputManager();

    void init(IPlatform& platform);
    void update();

    // ---- Per-player action queries ----

    [[nodiscard]] bool isHeld(Action action, int playerIndex = 0) const;
    [[nodiscard]] bool isJustPressed(Action action, int playerIndex = 0) const;
    [[nodiscard]] bool isJustReleased(Action action, int playerIndex = 0) const;

    // ---- Legacy P2-specific queries (backward compat) ----

    [[nodiscard]] bool isHeldP2(Action action) const { return isHeld(action, 1); }
    [[nodiscard]] bool isJustPressedP2(Action action) const { return isJustPressed(action, 1); }
    [[nodiscard]] bool isJustReleasedP2(Action action) const { return isJustReleased(action, 1); }

    // ---- Touch virtual buttons (for WASM/mobile overlay, P1 only) ----
    void setTouchButtonState(Action action, bool pressed);

    // ---- Key rebinding ----
    void bind(Action action, KeyCode key, int playerIndex = 0);
    [[nodiscard]] KeyCode getBinding(Action action, int playerIndex = 0) const;

    // ---- Mouse / touch position ----
    [[nodiscard]] Vec2f getPointerPosition() const;
    [[nodiscard]] bool isPointerDown() const;

private:
    static constexpr int ACTION_COUNT = static_cast<int>(Action::COUNT);

    void initDefaultBindings();

    IPlatform* m_platform = nullptr;

    // Per-player input state
    struct PlayerInput {
        std::array<bool, ACTION_COUNT> currState{};
        std::array<bool, ACTION_COUNT> prevState{};
        std::array<KeyCode, ACTION_COUNT> bindings{};
    };

    std::array<PlayerInput, MAX_PLAYERS> m_players;

    // Touch state (P1 only)
    std::array<bool, ACTION_COUNT> m_touchState{};
};
