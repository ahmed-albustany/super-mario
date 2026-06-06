#include "core/InputManager.hpp"
#include "utils/Logger.hpp"

InputManager::InputManager() {
    m_currState.fill(false);
    m_prevState.fill(false);
    m_touchState.fill(false);
    m_currStateP2.fill(false);
    m_prevStateP2.fill(false);
    initDefaultBindings();
}

void InputManager::init(IPlatform& platform) {
    m_platform = &platform;
}

void InputManager::initDefaultBindings() {
    // Player 1: Arrow keys + Z (jump) + X (run/fire)
    m_bindings[static_cast<int>(Action::MoveLeft)]  = KeyCode::Left;
    m_bindings[static_cast<int>(Action::MoveRight)] = KeyCode::Right;
    m_bindings[static_cast<int>(Action::MoveDown)]  = KeyCode::Down;
    m_bindings[static_cast<int>(Action::Jump)]      = KeyCode::Jump;
    m_bindings[static_cast<int>(Action::Run)]        = KeyCode::Run;
    m_bindings[static_cast<int>(Action::Pause)]     = KeyCode::Pause;
    m_bindings[static_cast<int>(Action::Confirm)]   = KeyCode::Confirm;
    m_bindings[static_cast<int>(Action::Back)]      = KeyCode::Back;

    // Player 2: WASD + J (jump) + K (run/fire)
    m_bindingsP2[static_cast<int>(Action::MoveLeft)]  = KeyCode::P2Left;
    m_bindingsP2[static_cast<int>(Action::MoveRight)] = KeyCode::P2Right;
    m_bindingsP2[static_cast<int>(Action::MoveDown)]  = KeyCode::P2Down;
    m_bindingsP2[static_cast<int>(Action::Jump)]      = KeyCode::P2Jump;
    m_bindingsP2[static_cast<int>(Action::Run)]        = KeyCode::P2Run;
    m_bindingsP2[static_cast<int>(Action::Pause)]     = KeyCode::Pause;
    m_bindingsP2[static_cast<int>(Action::Confirm)]   = KeyCode::Confirm;
    m_bindingsP2[static_cast<int>(Action::Back)]      = KeyCode::Back;
}

void InputManager::update() {
    if (!m_platform) return;

    // Save previous state
    m_prevState = m_currState;
    m_prevStateP2 = m_currStateP2;

    // Read raw state from platform + merge touch (Player 1)
    for (int i = 0; i < ACTION_COUNT; ++i) {
        KeyCode key = m_bindings[static_cast<std::size_t>(i)];
        bool fromKeyboard = m_platform->isKeyPressed(key);
        bool fromTouch = m_touchState[static_cast<size_t>(i)];
        m_currState[static_cast<size_t>(i)] = fromKeyboard || fromTouch;
    }

    // Read raw state for Player 2
    for (int i = 0; i < ACTION_COUNT; ++i) {
        KeyCode key = m_bindingsP2[static_cast<std::size_t>(i)];
        m_currStateP2[static_cast<size_t>(i)] = m_platform->isKeyPressed(key);
    }

    // ---- Input filtering: physically impossible combos ----
    // Left + Right simultaneously → resolve to neither (P1)
    bool left  = m_currState[static_cast<size_t>(Action::MoveLeft)];
    bool right = m_currState[static_cast<size_t>(Action::MoveRight)];
    if (left && right) {
        m_currState[static_cast<size_t>(Action::MoveLeft)]  = false;
        m_currState[static_cast<size_t>(Action::MoveRight)] = false;
    }

    // Same for P2
    bool left2  = m_currStateP2[static_cast<size_t>(Action::MoveLeft)];
    bool right2 = m_currStateP2[static_cast<size_t>(Action::MoveRight)];
    if (left2 && right2) {
        m_currStateP2[static_cast<size_t>(Action::MoveLeft)]  = false;
        m_currStateP2[static_cast<size_t>(Action::MoveRight)] = false;
    }
}

bool InputManager::isHeld(Action action) const {
    return m_currState[static_cast<size_t>(action)];
}

bool InputManager::isJustPressed(Action action) const {
    auto i = static_cast<size_t>(action);
    return m_currState[i] && !m_prevState[i];
}

bool InputManager::isJustReleased(Action action) const {
    auto i = static_cast<size_t>(action);
    return !m_currState[i] && m_prevState[i];
}

bool InputManager::isHeldP2(Action action) const {
    return m_currStateP2[static_cast<size_t>(action)];
}

bool InputManager::isJustPressedP2(Action action) const {
    auto i = static_cast<size_t>(action);
    return m_currStateP2[i] && !m_prevStateP2[i];
}

bool InputManager::isJustReleasedP2(Action action) const {
    auto i = static_cast<size_t>(action);
    return !m_currStateP2[i] && m_prevStateP2[i];
}

void InputManager::setTouchButtonState(Action action, bool pressed) {
    m_touchState[static_cast<size_t>(action)] = pressed;
}

void InputManager::bind(Action action, KeyCode key) {
    m_bindings[static_cast<std::size_t>(action)] = key;
}

KeyCode InputManager::getBinding(Action action) const {
    return m_bindings[static_cast<std::size_t>(action)];
}

Vec2f InputManager::getPointerPosition() const {
    if (!m_platform) return {0.0f, 0.0f};
    return m_platform->getTouchOrMousePosition();
}

bool InputManager::isPointerDown() const {
    if (!m_platform) return false;
    return m_platform->isTouchOrMouseDown();
}
