#pragma once

#include "platform/IPlatform.hpp"
#include "utils/Math.hpp"
#include <string>

/// @brief Styled text draw helper — no state, pure rendering.
///        Wraps font key lookup + position + size + color + alignment.
class Text {
public:
    /// @brief Text alignment relative to position.
    enum class Align { Left, Center, Right };

    /// @brief Construct styled text.
    /// @param text     The string to display.
    /// @param position Screen-space position (meaning depends on alignment).
    /// @param fontSize Size in pixels.
    /// @param color    Text color.
    /// @param align    Horizontal alignment relative to position.x.
    Text(const std::string& text, Vec2f position,
         unsigned int fontSize = 20,
         const Color& color = Color::White(),
         Align align = Align::Left);

    /// @brief Draw the text using the platform.
    /// @param platform Rendering target.
    /// @param font     Font handle to use.
    void draw(IPlatform& platform, FontHandle font) const;

    // ---- Mutators ----

    void setText(const std::string& text) { m_text = text; }
    void setPosition(Vec2f position) { m_position = position; }
    void setFontSize(unsigned int size) { m_fontSize = size; }
    void setColor(const Color& color) { m_color = color; }
    void setAlign(Align align) { m_align = align; }

    // ---- Accessors ----

    [[nodiscard]] const std::string& getText() const { return m_text; }
    [[nodiscard]] Vec2f getPosition() const { return m_position; }
    [[nodiscard]] unsigned int getFontSize() const { return m_fontSize; }

private:
    std::string  m_text;
    Vec2f        m_position;
    unsigned int m_fontSize;
    Color        m_color;
    Align        m_align;
};
