#include "ui/Button.hpp"

Button::Button(const Rect& bounds, const std::string& label,
               std::function<void()> callback)
    : m_bounds(bounds)
    , m_label(label)
    , m_callback(std::move(callback))
{}

bool Button::isClicked(Vec2f mousePos, bool mouseDown) {
    bool inside = m_bounds.contains(mousePos);

    if (mouseDown) {
        if (inside) {
            m_state = State::Pressed;
        }
    } else {
        if (inside) {
            // Mouse released inside — check if we were pressing
            if (m_wasDown && m_state == State::Pressed) {
                m_state = State::Hovered;
                m_wasDown = false;
                activate();
                return true;
            }
            m_state = State::Hovered;
        } else {
            m_state = State::Normal;
        }
    }

    m_wasDown = mouseDown;
    return false;
}

void Button::activate() {
    if (m_callback) {
        m_callback();
    }
}

void Button::draw(IPlatform& platform, FontHandle font) const {
    // Background color based on state
    Color bg;
    switch (m_state) {
        case State::Pressed: bg = m_pressedColor; break;
        case State::Hovered: bg = m_hoveredColor; break;
        default:             bg = m_normalColor;  break;
    }

    // Focus overrides hover for keyboard navigation
    if (m_focused && m_state != State::Pressed) {
        bg = m_hoveredColor;
    }

    // Border
    Color borderColor = m_focused ? m_focusColor : Color{70, 65, 85, 200};
    float borderWidth = m_focused ? 2.0f : 1.0f;

    platform.drawRect(m_bounds, bg, borderColor, borderWidth);

    // Label — centered horizontally, vertically offset for visual centering
    if (font.valid()) {
        float textX = m_bounds.x + m_bounds.w * 0.5f
                      - static_cast<float>(m_label.size()) * static_cast<float>(m_fontSize) * 0.3f;
        float textY = m_bounds.y + m_bounds.h * 0.5f - static_cast<float>(m_fontSize) * 0.5f;
        platform.drawText(font, m_label, {textX, textY}, m_fontSize, m_textColor);
    }
}

void Button::setColors(const Color& normal, const Color& hovered, const Color& pressed) {
    m_normalColor  = normal;
    m_hoveredColor = hovered;
    m_pressedColor = pressed;
}
