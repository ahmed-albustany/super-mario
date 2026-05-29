#pragma once

#include "platform/IPlatform.hpp"
#include "utils/Math.hpp"
#include <string>
#include <functional>

/// @brief Reusable UI button with Normal/Hovered/Pressed states.
///        Supports keyboard focus highlight and mouse/touch click detection.
///        Accepts a callback on construction, invoked on release inside bounds.
class Button {
public:
    enum class State { Normal, Hovered, Pressed };

    /// @brief Construct a button.
    /// @param bounds   Position and size in screen space.
    /// @param label    Text displayed centered on the button.
    /// @param callback Called when the button is clicked (released inside bounds).
    Button(const Rect& bounds, const std::string& label,
           std::function<void()> callback = nullptr);

    /// @brief Test mouse interaction. Returns true on release inside bounds.
    /// @param mousePos  Current mouse/touch position in screen space.
    /// @param mouseDown True if mouse button / touch is currently held.
    bool isClicked(Vec2f mousePos, bool mouseDown);

    /// @brief Set keyboard focus state (draws highlight border).
    void setFocused(bool focused) { m_focused = focused; }

    /// @brief Draw the button using the platform.
    /// @param platform Rendering target.
    /// @param font     Font handle for label text.
    void draw(IPlatform& platform, FontHandle font) const;

    /// @brief Fire the callback (used by keyboard Confirm).
    void activate();

    // ---- Accessors ----

    void setLabel(const std::string& label) { m_label = label; }
    [[nodiscard]] const std::string& getLabel() const { return m_label; }
    void setBounds(const Rect& bounds) { m_bounds = bounds; }
    [[nodiscard]] const Rect& getBounds() const { return m_bounds; }
    [[nodiscard]] State getState() const { return m_state; }
    [[nodiscard]] bool isFocused() const { return m_focused; }

    // ---- Style ----

    void setColors(const Color& normal, const Color& hovered, const Color& pressed);
    void setTextColor(const Color& color) { m_textColor = color; }
    void setFocusColor(const Color& color) { m_focusColor = color; }
    void setFontSize(unsigned int size) { m_fontSize = size; }

private:
    Rect m_bounds;
    std::string m_label;
    std::function<void()> m_callback;

    State m_state   = State::Normal;
    bool  m_focused = false;
    bool  m_wasDown = false;  // tracks previous frame for release detection

    // Style
    Color m_normalColor  = Color{40, 35, 55, 200};
    Color m_hoveredColor = Color{60, 50, 80, 220};
    Color m_pressedColor = Color{80, 60, 120, 240};
    Color m_textColor    = Color{255, 220, 150, 255};
    Color m_focusColor   = Color{180, 140, 220, 255};
    unsigned int m_fontSize = 20;
};
