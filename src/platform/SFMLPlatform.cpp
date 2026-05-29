#include "platform/SFMLPlatform.hpp"
#include "core/GameConfig.hpp"
#include "utils/Logger.hpp"

// =============================================================================
// Factory
// =============================================================================
std::unique_ptr<IPlatform> createSFMLPlatform() {
    return std::make_unique<SFMLPlatform>();
}

// =============================================================================
// Construction / Destruction
// =============================================================================
SFMLPlatform::SFMLPlatform()
    : m_window(sf::VideoMode(
          static_cast<unsigned>(Config::WINDOW_WIDTH),
          static_cast<unsigned>(Config::WINDOW_HEIGHT)),
        Config::GAME_TITLE,
        sf::Style::Close | sf::Style::Titlebar)
{
    m_window.setFramerateLimit(static_cast<unsigned>(Config::TARGET_FPS));
    m_window.setVerticalSyncEnabled(false);
    m_window.setKeyRepeatEnabled(false);
    initKeyMap();
    m_currKeys.fill(false);
    m_prevKeys.fill(false);
    LOG_INFO("SFMLPlatform initialized: " << Config::WINDOW_WIDTH << "x" << Config::WINDOW_HEIGHT);
}

SFMLPlatform::~SFMLPlatform() {
    if (m_window.isOpen()) {
        m_window.close();
    }
}

// =============================================================================
// Key mapping
// =============================================================================
void SFMLPlatform::initKeyMap() {
    m_keyMap[static_cast<int>(KeyCode::Up)]      = {sf::Keyboard::Up,     sf::Keyboard::W};
    m_keyMap[static_cast<int>(KeyCode::Down)]     = {sf::Keyboard::Down,   sf::Keyboard::S};
    m_keyMap[static_cast<int>(KeyCode::Left)]     = {sf::Keyboard::Left,   sf::Keyboard::A};
    m_keyMap[static_cast<int>(KeyCode::Right)]    = {sf::Keyboard::Right,  sf::Keyboard::D};
    m_keyMap[static_cast<int>(KeyCode::Jump)]     = {sf::Keyboard::Z,      sf::Keyboard::Space};
    m_keyMap[static_cast<int>(KeyCode::Dash)]     = {sf::Keyboard::X,      sf::Keyboard::LShift};
    m_keyMap[static_cast<int>(KeyCode::Pause)]    = {sf::Keyboard::Escape, sf::Keyboard::P};
    m_keyMap[static_cast<int>(KeyCode::Confirm)]  = {sf::Keyboard::Return, sf::Keyboard::Z};
    m_keyMap[static_cast<int>(KeyCode::Back)]     = {sf::Keyboard::Escape, sf::Keyboard::X};
    m_keyMap[static_cast<int>(KeyCode::Debug)]    = {sf::Keyboard::F1,     sf::Keyboard::Unknown};
}

sf::Color SFMLPlatform::toSFColor(const Color& c) {
    return sf::Color(c.r, c.g, c.b, c.a);
}

// =============================================================================
// Window lifecycle
// =============================================================================
void SFMLPlatform::pollEvents() {
    // Swap key state
    m_prevKeys = m_currKeys;

    // Process SFML events
    sf::Event event{};
    while (m_window.pollEvent(event)) {
        switch (event.type) {
            case sf::Event::Closed:
                m_running = false;
                break;
            case sf::Event::MouseButtonPressed:
                if (event.mouseButton.button == sf::Mouse::Left) {
                    m_mouseDown = true;
                }
                break;
            case sf::Event::MouseButtonReleased:
                if (event.mouseButton.button == sf::Mouse::Left) {
                    m_mouseDown = false;
                }
                break;
            case sf::Event::MouseMoved:
                m_mousePos = {static_cast<float>(event.mouseMove.x),
                              static_cast<float>(event.mouseMove.y)};
                break;
            default:
                break;
        }
    }

    // Sample current key state from SFML's real-time input
    for (int i = 0; i < KEY_COUNT; ++i) {
        const auto& binding = m_keyMap[i];
        bool pressed = false;
        if (binding.primary != sf::Keyboard::Unknown) {
            pressed = sf::Keyboard::isKeyPressed(binding.primary);
        }
        if (!pressed && binding.secondary != sf::Keyboard::Unknown) {
            pressed = sf::Keyboard::isKeyPressed(binding.secondary);
        }
        m_currKeys[static_cast<size_t>(i)] = pressed;
    }

    // Update delta time
    m_deltaTime = m_clock.restart().asSeconds();
    if (m_deltaTime > Config::MAX_DELTA_TIME) {
        m_deltaTime = Config::MAX_DELTA_TIME;
    }
}

bool SFMLPlatform::isRunning() const {
    return m_running && m_window.isOpen();
}

void SFMLPlatform::close() {
    m_running = false;
    m_window.close();
}

// =============================================================================
// Rendering
// =============================================================================
void SFMLPlatform::clear(const Color& color) {
    m_window.clear(toSFColor(color));
}

void SFMLPlatform::display() {
    m_window.display();
}

void SFMLPlatform::drawSprite(TextureHandle texture, const Rect& srcRect,
                               const Vec2f& position, const Vec2f& scale,
                               float rotation, bool flipX, const Color& tint) {
    auto it = m_textures.find(texture.id);
    if (it == m_textures.end() || !it->second) return;

    sf::Sprite sprite(*it->second);
    sprite.setTextureRect(sf::IntRect(
        static_cast<int>(srcRect.x), static_cast<int>(srcRect.y),
        static_cast<int>(srcRect.w), static_cast<int>(srcRect.h)
    ));

    // Apply camera offset
    Vec2f drawPos = position - m_cameraOffset;
    sprite.setPosition(drawPos.x, drawPos.y);

    float sx = flipX ? -scale.x : scale.x;
    sprite.setScale(sx, scale.y);

    if (flipX) {
        // When flipped, set origin to right edge so sprite flips in place
        sprite.setOrigin(srcRect.w, 0.0f);
    } else {
        sprite.setOrigin(0.0f, 0.0f);
    }

    sprite.setRotation(rotation);
    sprite.setColor(toSFColor(tint));
    m_window.draw(sprite);
}

void SFMLPlatform::drawRect(const Rect& rect, const Color& fillColor,
                              const Color& outlineColor, float outlineThickness) {
    sf::RectangleShape shape(sf::Vector2f(rect.w, rect.h));
    shape.setPosition(rect.x - m_cameraOffset.x, rect.y - m_cameraOffset.y);
    shape.setFillColor(toSFColor(fillColor));

    if (outlineThickness > 0.0f) {
        shape.setOutlineColor(toSFColor(outlineColor));
        shape.setOutlineThickness(outlineThickness);
    }

    m_window.draw(shape);
}

void SFMLPlatform::drawText(FontHandle font, const std::string& text,
                              const Vec2f& position, unsigned int size,
                              const Color& color) {
    auto it = m_fonts.find(font.id);
    if (it == m_fonts.end() || !it->second) return;

    sf::Text sfText;
    sfText.setFont(*it->second);
    sfText.setString(text);
    sfText.setCharacterSize(size);
    sfText.setFillColor(toSFColor(color));
    sfText.setPosition(position.x, position.y); // UI text: no camera offset
    m_window.draw(sfText);
}

// =============================================================================
// Timing
// =============================================================================
float SFMLPlatform::getDeltaTime() const {
    return m_deltaTime;
}

// =============================================================================
// Window info
// =============================================================================
Vec2i SFMLPlatform::getWindowSize() const {
    auto size = m_window.getSize();
    return {static_cast<int>(size.x), static_cast<int>(size.y)};
}

// =============================================================================
// Input
// =============================================================================
bool SFMLPlatform::isKeyPressed(KeyCode key) const {
    return m_currKeys[static_cast<size_t>(key)];
}

bool SFMLPlatform::isKeyJustPressed(KeyCode key) const {
    auto i = static_cast<size_t>(key);
    return m_currKeys[i] && !m_prevKeys[i];
}

bool SFMLPlatform::isKeyJustReleased(KeyCode key) const {
    auto i = static_cast<size_t>(key);
    return !m_currKeys[i] && m_prevKeys[i];
}

Vec2f SFMLPlatform::getTouchOrMousePosition() const {
    return m_mousePos;
}

bool SFMLPlatform::isTouchOrMouseDown() const {
    return m_mouseDown;
}

// =============================================================================
// Audio
// =============================================================================
void SFMLPlatform::playSound(SoundHandle sound) {
    auto it = m_soundBuffers.find(sound.id);
    if (it == m_soundBuffers.end() || !it->second) return;

    // Find a free slot in the sound pool
    for (auto& slot : m_soundPool) {
        if (slot.getStatus() == sf::Sound::Stopped) {
            slot.setBuffer(*it->second);
            slot.setVolume(m_sfxVolume);
            slot.play();
            return;
        }
    }
    // All slots busy — drop the sound (no crash, no allocation)
    LOG_DEBUG("Sound pool full, dropping sound");
}

void SFMLPlatform::playMusic(const std::string& path, bool loop) {
    m_music.stop();
    if (!m_music.openFromFile(path)) {
        LOG_ERROR("Failed to open music file");
        return;
    }
    m_music.setLoop(loop);
    m_music.play();
}

void SFMLPlatform::stopMusic() {
    m_music.stop();
}

void SFMLPlatform::setMusicVolume(float volume) {
    m_music.setVolume(Math::clamp(volume, 0.0f, 1.0f) * 100.0f);
}

void SFMLPlatform::setSfxVolume(float volume) {
    m_sfxVolume = Math::clamp(volume, 0.0f, 1.0f) * 100.0f;
}

// =============================================================================
// Resource loading
// =============================================================================
TextureHandle SFMLPlatform::loadTexture(const std::string& key,
                                          const std::string& path) {
    (void)key; // key used for logging only at this layer
    auto tex = std::make_unique<sf::Texture>();
    if (!tex->loadFromFile(path)) {
        LOG_ERROR("Failed to load texture: " << key);
        return TextureHandle{0};
    }
    uint32_t id = m_nextTexId++;
    m_textures[id] = std::move(tex);
    LOG_DEBUG("Loaded texture '" << key << "' as handle " << id);
    return TextureHandle{id};
}

SoundHandle SFMLPlatform::loadSound(const std::string& key,
                                      const std::string& path) {
    (void)key;
    auto buf = std::make_unique<sf::SoundBuffer>();
    if (!buf->loadFromFile(path)) {
        LOG_ERROR("Failed to load sound: " << key);
        return SoundHandle{0};
    }
    uint32_t id = m_nextSndId++;
    m_soundBuffers[id] = std::move(buf);
    LOG_DEBUG("Loaded sound '" << key << "' as handle " << id);
    return SoundHandle{id};
}

FontHandle SFMLPlatform::loadFont(const std::string& key,
                                    const std::string& path) {
    (void)key;
    auto font = std::make_unique<sf::Font>();
    if (!font->loadFromFile(path)) {
        LOG_ERROR("Failed to load font: " << key);
        return FontHandle{0};
    }
    uint32_t id = m_nextFontId++;
    m_fonts[id] = std::move(font);
    LOG_DEBUG("Loaded font '" << key << "' as handle " << id);
    return FontHandle{id};
}

// =============================================================================
// Camera
// =============================================================================
void SFMLPlatform::setCameraOffset(const Vec2f& offset) {
    m_cameraOffset = offset;
}

Vec2f SFMLPlatform::getCameraOffset() const {
    return m_cameraOffset;
}
