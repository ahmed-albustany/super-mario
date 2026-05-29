#include "ui/Text.hpp"

Text::Text(const std::string& text, Vec2f position,
           unsigned int fontSize, const Color& color, Align align)
    : m_text(text)
    , m_position(position)
    , m_fontSize(fontSize)
    , m_color(color)
    , m_align(align)
{}

void Text::draw(IPlatform& platform, FontHandle font) const {
    if (!font.valid() || m_text.empty()) return;

    // Estimate text width for alignment (approximate: charCount * fontSize * 0.6)
    float estimatedWidth = static_cast<float>(m_text.size())
                           * static_cast<float>(m_fontSize) * 0.6f;

    Vec2f drawPos = m_position;

    switch (m_align) {
        case Align::Center:
            drawPos.x -= estimatedWidth * 0.5f;
            break;
        case Align::Right:
            drawPos.x -= estimatedWidth;
            break;
        case Align::Left:
            break;
    }

    platform.drawText(font, m_text, drawPos, m_fontSize, m_color);
}
