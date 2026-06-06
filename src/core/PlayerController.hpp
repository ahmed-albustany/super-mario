#pragma once

#include "core/InputManager.hpp"

/// @brief Per-player input abstraction.
///        Reads from the correct player slot in InputManager based on playerIndex.
///        Game systems use this instead of querying InputManager directly,
///        so they never need to know which key bindings belong to which player.
class PlayerController {
public:
    explicit PlayerController(int playerIndex = 0) : m_playerIndex(playerIndex) {}

    /// @brief Read current input state from the InputManager.
    void update(const InputManager& input) {
        if (m_playerIndex == 0) {
            m_moveX = 0.0f;
            if (input.isHeld(Action::MoveLeft))  m_moveX -= 1.0f;
            if (input.isHeld(Action::MoveRight)) m_moveX += 1.0f;
            m_moveDown     = input.isHeld(Action::MoveDown);
            m_jumpPressed  = input.isJustPressed(Action::Jump);
            m_jumpHeld     = input.isHeld(Action::Jump);
            m_jumpReleased = input.isJustReleased(Action::Jump);
            m_runHeld      = input.isHeld(Action::Run);
            m_runPressed   = input.isJustPressed(Action::Run);
        } else {
            m_moveX = 0.0f;
            if (input.isHeldP2(Action::MoveLeft))  m_moveX -= 1.0f;
            if (input.isHeldP2(Action::MoveRight)) m_moveX += 1.0f;
            m_moveDown     = input.isHeldP2(Action::MoveDown);
            m_jumpPressed  = input.isJustPressedP2(Action::Jump);
            m_jumpHeld     = input.isHeldP2(Action::Jump);
            m_jumpReleased = input.isJustReleasedP2(Action::Jump);
            m_runHeld      = input.isHeldP2(Action::Run);
            m_runPressed   = input.isJustPressedP2(Action::Run);
        }
    }

    // ---- Queries (valid after update()) ----

    [[nodiscard]] float moveX()        const { return m_moveX; }
    [[nodiscard]] bool  moveDown()     const { return m_moveDown; }
    [[nodiscard]] bool  jumpPressed()  const { return m_jumpPressed; }
    [[nodiscard]] bool  jumpHeld()     const { return m_jumpHeld; }
    [[nodiscard]] bool  jumpReleased() const { return m_jumpReleased; }
    [[nodiscard]] bool  runHeld()      const { return m_runHeld; }
    [[nodiscard]] bool  runPressed()   const { return m_runPressed; }

    [[nodiscard]] int   playerIndex()  const { return m_playerIndex; }
    void setPlayerIndex(int idx)              { m_playerIndex = idx; }

private:
    int   m_playerIndex  = 0;
    float m_moveX        = 0.0f;
    bool  m_moveDown     = false;
    bool  m_jumpPressed  = false;
    bool  m_jumpHeld     = false;
    bool  m_jumpReleased = false;
    bool  m_runHeld      = false;
    bool  m_runPressed   = false;
};
