#include "ui/Panel.hpp"

Panel::Panel(const Rect& bounds, const Color& fillColor)
    : m_bounds(bounds)
    , m_fillColor(fillColor)
{}

void Panel::draw(IPlatform& platform, FontHandle font) const {
    // Draw filled background with optional border
    platform.drawRect(m_bounds, m_fillColor, m_borderColor, m_borderWidth);

    // Draw title text at top-center of panel
    if (!m_title.empty() && font.valid()) {
        float titleX = m_bounds.x + m_bounds.w * 0.5f
                       - static_cast<float>(m_title.size()) * static_cast<float>(m_titleSize) * 0.3f;
        float titleY = m_bounds.y + 12.0f;
        platform.drawText(font, m_title, {titleX, titleY}, m_titleSize, m_titleColor);
    }
}
