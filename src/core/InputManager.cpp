#include "core/InputManager.hpp"
#include "utils/Logger.hpp"

InputManager::InputManager() {
    m_touchState.fill(false);
    for (auto& p : m_players) {
        p.currState.fill(false);
        p.prevState.fill(false);
    }
    initDefaultBindings();
}

void InputManager::init(IPlatform& platform) {
    m_platform = &platform;
}

void InputManager::initDefaultBindings() {
    auto idx = [](Action a) { return static_cast<int>(a); };

    // Player 1: Arrow keys + Z jump + X double jump
    m_players[0].bindings[idx(Action::MoveLeft)]    = KeyCode::Left;
    m_players[0].bindings[idx(Action::MoveRight)]   = KeyCode::Right;
    m_players[0].bindings[idx(Action::MoveDown)]    = KeyCode::Down;
    m_players[0].bindings[idx(Action::Jump)]        = KeyCode::Jump;      // Z
    m_players[0].bindings[idx(Action::DoubleJump)]  = KeyCode::Run;       // X
    m_players[0].bindings[idx(Action::Pause)]       = KeyCode::Pause;
    m_players[0].bindings[idx(Action::Confirm)]     = KeyCode::Confirm;
    m_players[0].bindings[idx(Action::Back)]        = KeyCode::Back;

    // Player 2: WASD + J jump + K double jump
    m_players[1].bindings[idx(Action::MoveLeft)]    = KeyCode::P2Left;
    m_players[1].bindings[idx(Action::MoveRight)]   = KeyCode::P2Right;
    m_players[1].bindings[idx(Action::MoveDown)]    = KeyCode::P2Down;
    m_players[1].bindings[idx(Action::Jump)]        = KeyCode::P2Jump;    // J
    m_players[1].bindings[idx(Action::DoubleJump)]  = KeyCode::P2Run;     // K
    m_players[1].bindings[idx(Action::Pause)]       = KeyCode::Pause;
    m_players[1].bindings[idx(Action::Confirm)]     = KeyCode::Confirm;
    m_players[1].bindings[idx(Action::Back)]        = KeyCode::Back;

    // Player 3: IJKL + N jump + M double jump
    m_players[2].bindings[idx(Action::MoveLeft)]    = KeyCode::P3Left;    // J (reuses P3Left)
    m_players[2].bindings[idx(Action::MoveRight)]   = KeyCode::P3Right;   // L
    m_players[2].bindings[idx(Action::MoveDown)]    = KeyCode::P3Down;    // K
    m_players[2].bindings[idx(Action::Jump)]        = KeyCode::P3Jump;    // N
    m_players[2].bindings[idx(Action::DoubleJump)]  = KeyCode::P3Run;     // M
    m_players[2].bindings[idx(Action::Pause)]       = KeyCode::Pause;
    m_players[2].bindings[idx(Action::Confirm)]     = KeyCode::Confirm;
    m_players[2].bindings[idx(Action::Back)]        = KeyCode::Back;

    // Player 4: Numpad 8456 + Numpad1 jump + Numpad2 double jump
    m_players[3].bindings[idx(Action::MoveLeft)]    = KeyCode::P4Left;    // Numpad4
    m_players[3].bindings[idx(Action::MoveRight)]   = KeyCode::P4Right;   // Numpad6
    m_players[3].bindings[idx(Action::MoveDown)]    = KeyCode::P4Down;    // Numpad5
    m_players[3].bindings[idx(Action::Jump)]        = KeyCode::P4Jump;    // Numpad1
    m_players[3].bindings[idx(Action::DoubleJump)]  = KeyCode::P4Run;     // Numpad2
    m_players[3].bindings[idx(Action::Pause)]       = KeyCode::Pause;
    m_players[3].bindings[idx(Action::Confirm)]     = KeyCode::Confirm;
    m_players[3].bindings[idx(Action::Back)]        = KeyCode::Back;
}

void InputManager::update() {
    if (!m_platform) return;

    for (int p = 0; p < MAX_PLAYERS; ++p) {
        auto& pi = m_players[static_cast<size_t>(p)];

        // Save previous state
        pi.prevState = pi.currState;

        // Read raw state from platform
        for (int i = 0; i < ACTION_COUNT; ++i) {
            KeyCode key = pi.bindings[static_cast<size_t>(i)];
            bool fromKeyboard = m_platform->isKeyPressed(key);
            // Merge touch input for P1 only
            bool fromTouch = (p == 0) ? m_touchState[static_cast<size_t>(i)] : false;
            pi.currState[static_cast<size_t>(i)] = fromKeyboard || fromTouch;
        }

        // Input filtering: Left + Right simultaneously → resolve to neither
        bool left  = pi.currState[static_cast<size_t>(Action::MoveLeft)];
        bool right = pi.currState[static_cast<size_t>(Action::MoveRight)];
        if (left && right) {
            pi.currState[static_cast<size_t>(Action::MoveLeft)]  = false;
            pi.currState[static_cast<size_t>(Action::MoveRight)] = false;
        }
    }
}

bool InputManager::isHeld(Action action, int playerIndex) const {
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return false;
    return m_players[static_cast<size_t>(playerIndex)].currState[static_cast<size_t>(action)];
}

bool InputManager::isJustPressed(Action action, int playerIndex) const {
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return false;
    auto i = static_cast<size_t>(action);
    return m_players[static_cast<size_t>(playerIndex)].currState[i] &&
           !m_players[static_cast<size_t>(playerIndex)].prevState[i];
}

bool InputManager::isJustReleased(Action action, int playerIndex) const {
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return false;
    auto i = static_cast<size_t>(action);
    return !m_players[static_cast<size_t>(playerIndex)].currState[i] &&
            m_players[static_cast<size_t>(playerIndex)].prevState[i];
}

void InputManager::setTouchButtonState(Action action, bool pressed) {
    m_touchState[static_cast<size_t>(action)] = pressed;
}

void InputManager::bind(Action action, KeyCode key, int playerIndex) {
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return;
    m_players[static_cast<size_t>(playerIndex)].bindings[static_cast<size_t>(action)] = key;
}

KeyCode InputManager::getBinding(Action action, int playerIndex) const {
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return KeyCode::COUNT;
    return m_players[static_cast<size_t>(playerIndex)].bindings[static_cast<size_t>(action)];
}

Vec2f InputManager::getPointerPosition() const {
    if (!m_platform) return {0.0f, 0.0f};
    return m_platform->getTouchOrMousePosition();
}

bool InputManager::isPointerDown() const {
    if (!m_platform) return false;
    return m_platform->isTouchOrMouseDown();
}
