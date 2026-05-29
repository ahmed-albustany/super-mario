#pragma once

#include "platform/IPlatform.hpp"
#include "utils/Math.hpp"
#include <string>

/// @brief Background panel — draws a filled rect with optional border and title.
///        Supports alpha for semi-transparent overlays (e.g. PauseScene dimming).
class Panel {
public:
    /// @brief Construct a panel.
    /// @param bounds    Position and size in screen space.
    /// @param fillColor Background fill (alpha < 255 for transparency).
    Panel(const Rect& bounds, const Color& fillColor = Color{0, 0, 0, 180});

    /// @brief Draw the panel using the platform.
    /// @param platform Rendering target.
    /// @param font     Font handle for optional title text (can be invalid if no title).
    void draw(IPlatform& platform, FontHandle font = FontHandle{0}) const;

    // ---- Configuration ----

    void setBounds(const Rect& bounds) { m_bounds = bounds; }
    [[nodiscard]] const Rect& getBounds() const { return m_bounds; }

    void setFillColor(const Color& color) { m_fillColor = color; }
    void setBorderColor(const Color& color) { m_borderColor = color; }
    void setBorderWidth(float width) { m_borderWidth = width; }

    void setTitle(const std::string& title) { m_title = title; }
    void setTitleColor(const Color& color) { m_titleColor = color; }
    void setTitleSize(unsigned int size) { m_titleSize = size; }

private:
    Rect  m_bounds;
    Color m_fillColor;
    Color m_borderColor  = Color{100, 90, 130, 200};
    float m_borderWidth  = 1.0f;

    std::string  m_title;
    Color        m_titleColor = Color{255, 255, 255, 255};
    unsigned int m_titleSize  = 24;
};
