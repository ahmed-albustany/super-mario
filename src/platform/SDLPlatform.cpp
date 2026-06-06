#ifdef MARIO_WASM

#include "platform/SDLPlatform.hpp"
#include "core/GameConfig.hpp"
#include "utils/Logger.hpp"

// =============================================================================
// Factory
// =============================================================================
std::unique_ptr<IPlatform> createSDLPlatform() {
    return std::make_unique<SDLPlatform>();
}

// =============================================================================
// Construction / Destruction
// =============================================================================
SDLPlatform::SDLPlatform() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        LOG_FATAL("SDL_Init failed: " << SDL_GetError());
    }

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        LOG_FATAL("IMG_Init failed: " << IMG_GetError());
    }

    if (TTF_Init() != 0) {
        LOG_FATAL("TTF_Init failed: " << TTF_GetError());
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        LOG_ERROR("Mix_OpenAudio failed: " << Mix_GetError());
    }
    Mix_AllocateChannels(Config::MAX_SOUND_CHANNELS);

    m_window = SDL_CreateWindow(
        Config::GAME_TITLE.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!m_window) {
        LOG_FATAL("SDL_CreateWindow failed: " << SDL_GetError());
    }

    // In WASM, vsync is handled by the browser via requestAnimationFrame
    // (emscripten_set_main_loop with fps=0). SDL_RENDERER_PRESENTVSYNC would
    // trigger an incompatible emscripten_set_main_loop_timing call.
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
    if (!m_renderer) {
        LOG_FATAL("SDL_CreateRenderer failed: " << SDL_GetError());
    }

    SDL_RenderSetLogicalSize(m_renderer, Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT);

    initKeyMap();
    m_currKeys.fill(false);
    m_prevKeys.fill(false);

    m_perfFreq = SDL_GetPerformanceFrequency();
    m_lastTime = SDL_GetPerformanceCounter();

    LOG_INFO("SDLPlatform initialized (WASM mode)");
}

SDLPlatform::~SDLPlatform() {
    // Free textures
    for (auto& [id, data] : m_textures) {
        if (data.texture) SDL_DestroyTexture(data.texture);
    }

    // Free sounds
    for (auto& [id, chunk] : m_sounds) {
        if (chunk) Mix_FreeChunk(chunk);
    }

    // Free fonts
    for (auto& [id, fontData] : m_fonts) {
        for (auto& [size, font] : fontData.sizedFonts) {
            if (font) TTF_CloseFont(font);
        }
    }

    // Free music
    if (m_music) {
        Mix_FreeMusic(m_music);
    }

    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window) SDL_DestroyWindow(m_window);

    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

// =============================================================================
// Key mapping
// =============================================================================
void SDLPlatform::initKeyMap() {
    // Player 1: Arrow keys + Z (jump) + X (run/fire)
    m_keyMap[static_cast<int>(KeyCode::Up)]      = {SDL_SCANCODE_UP,      SDL_SCANCODE_UNKNOWN};
    m_keyMap[static_cast<int>(KeyCode::Down)]     = {SDL_SCANCODE_DOWN,    SDL_SCANCODE_UNKNOWN};
    m_keyMap[static_cast<int>(KeyCode::Left)]     = {SDL_SCANCODE_LEFT,    SDL_SCANCODE_UNKNOWN};
    m_keyMap[static_cast<int>(KeyCode::Right)]    = {SDL_SCANCODE_RIGHT,   SDL_SCANCODE_UNKNOWN};
    m_keyMap[static_cast<int>(KeyCode::Jump)]     = {SDL_SCANCODE_Z,       SDL_SCANCODE_SPACE};
    m_keyMap[static_cast<int>(KeyCode::Run)]      = {SDL_SCANCODE_X,       SDL_SCANCODE_LSHIFT};
    m_keyMap[static_cast<int>(KeyCode::Pause)]    = {SDL_SCANCODE_ESCAPE,  SDL_SCANCODE_P};
    m_keyMap[static_cast<int>(KeyCode::Confirm)]  = {SDL_SCANCODE_RETURN,  SDL_SCANCODE_Z};
    m_keyMap[static_cast<int>(KeyCode::Back)]     = {SDL_SCANCODE_ESCAPE,  SDL_SCANCODE_X};
    m_keyMap[static_cast<int>(KeyCode::Debug)]    = {SDL_SCANCODE_F1,      SDL_SCANCODE_UNKNOWN};

    // Player 2: WASD + J (jump) + K (run/fire)
    m_keyMap[static_cast<int>(KeyCode::P2Up)]    = {SDL_SCANCODE_W,       SDL_SCANCODE_UNKNOWN};
    m_keyMap[static_cast<int>(KeyCode::P2Down)]   = {SDL_SCANCODE_S,       SDL_SCANCODE_UNKNOWN};
    m_keyMap[static_cast<int>(KeyCode::P2Left)]   = {SDL_SCANCODE_A,       SDL_SCANCODE_UNKNOWN};
    m_keyMap[static_cast<int>(KeyCode::P2Right)]  = {SDL_SCANCODE_D,       SDL_SCANCODE_UNKNOWN};
    m_keyMap[static_cast<int>(KeyCode::P2Jump)]   = {SDL_SCANCODE_J,       SDL_SCANCODE_UNKNOWN};
    m_keyMap[static_cast<int>(KeyCode::P2Run)]    = {SDL_SCANCODE_K,       SDL_SCANCODE_UNKNOWN};
}

// =============================================================================
// Window lifecycle
// =============================================================================
void SDLPlatform::pollEvents() {
    m_prevKeys = m_currKeys;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                m_running = false;
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    m_touchDown = true;
                    m_touchPos = {static_cast<float>(event.button.x),
                                  static_cast<float>(event.button.y)};
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    m_touchDown = false;
                }
                break;

            case SDL_MOUSEMOTION:
                m_touchPos = {static_cast<float>(event.motion.x),
                              static_cast<float>(event.motion.y)};
                break;

            case SDL_FINGERDOWN:
                m_touchDown = true;
                m_touchPos = {event.tfinger.x * static_cast<float>(Config::WINDOW_WIDTH),
                              event.tfinger.y * static_cast<float>(Config::WINDOW_HEIGHT)};
                break;

            case SDL_FINGERUP:
                m_touchDown = false;
                break;

            case SDL_FINGERMOTION:
                m_touchPos = {event.tfinger.x * static_cast<float>(Config::WINDOW_WIDTH),
                              event.tfinger.y * static_cast<float>(Config::WINDOW_HEIGHT)};
                break;

            default:
                break;
        }
    }

    // Sample keyboard state
    const Uint8* state = SDL_GetKeyboardState(nullptr);
    for (int i = 0; i < KEY_COUNT; ++i) {
        const auto& binding = m_keyMap[i];
        bool pressed = false;
        if (binding.primary != SDL_SCANCODE_UNKNOWN) {
            pressed = (state[binding.primary] != 0);
        }
        if (!pressed && binding.secondary != SDL_SCANCODE_UNKNOWN) {
            pressed = (state[binding.secondary] != 0);
        }
        m_currKeys[static_cast<size_t>(i)] = pressed;
    }

    // Update delta time
    Uint64 now = SDL_GetPerformanceCounter();
    m_deltaTime = static_cast<float>(now - m_lastTime) / static_cast<float>(m_perfFreq);
    m_lastTime = now;
    if (m_deltaTime > Config::MAX_DELTA_TIME) {
        m_deltaTime = Config::MAX_DELTA_TIME;
    }
}

bool SDLPlatform::isRunning() const {
    return m_running;
}

void SDLPlatform::close() {
    m_running = false;
}

// =============================================================================
// Rendering
// =============================================================================
void SDLPlatform::clear(const Color& color) {
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(m_renderer);
}

void SDLPlatform::display() {
    SDL_RenderPresent(m_renderer);
}

void SDLPlatform::drawSprite(TextureHandle texture, const Rect& srcRect,
                               const Vec2f& position, const Vec2f& scale,
                               float rotation, bool flipX, const Color& tint) {
    auto it = m_textures.find(texture.id);
    if (it == m_textures.end() || !it->second.texture) return;

    SDL_Texture* tex = it->second.texture;

    SDL_Rect src;
    src.x = static_cast<int>(srcRect.x);
    src.y = static_cast<int>(srcRect.y);
    src.w = static_cast<int>(srcRect.w);
    src.h = static_cast<int>(srcRect.h);

    Vec2f drawPos = position - m_cameraOffset;

    SDL_Rect dst;
    dst.x = static_cast<int>(drawPos.x);
    dst.y = static_cast<int>(drawPos.y);
    dst.w = static_cast<int>(srcRect.w * std::abs(scale.x));
    dst.h = static_cast<int>(srcRect.h * scale.y);

    SDL_SetTextureColorMod(tex, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(tex, tint.a);

    SDL_RendererFlip flip = flipX ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

    SDL_RenderCopyEx(m_renderer, tex, &src, &dst,
                     static_cast<double>(rotation),
                     nullptr, flip);
}

void SDLPlatform::drawRect(const Rect& rect, const Color& fillColor,
                             const Color& outlineColor, float outlineThickness) {
    SDL_Rect r;
    r.x = static_cast<int>(rect.x - m_cameraOffset.x);
    r.y = static_cast<int>(rect.y - m_cameraOffset.y);
    r.w = static_cast<int>(rect.w);
    r.h = static_cast<int>(rect.h);

    // Fill
    if (fillColor.a > 0) {
        SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(m_renderer, fillColor.r, fillColor.g,
                               fillColor.b, fillColor.a);
        SDL_RenderFillRect(m_renderer, &r);
    }

    // Outline
    if (outlineThickness > 0.0f && outlineColor.a > 0) {
        SDL_SetRenderDrawColor(m_renderer, outlineColor.r, outlineColor.g,
                               outlineColor.b, outlineColor.a);
        // Draw multiple outlines for thickness > 1
        int thick = static_cast<int>(outlineThickness);
        for (int i = 0; i < thick; ++i) {
            SDL_Rect outline = {r.x - i, r.y - i,
                                r.w + 2 * i, r.h + 2 * i};
            SDL_RenderDrawRect(m_renderer, &outline);
        }
    }
}

void SDLPlatform::drawText(FontHandle font, const std::string& text,
                             const Vec2f& position, unsigned int size,
                             const Color& color) {
    TTF_Font* ttf = getOrOpenFont(font.id, size);
    if (!ttf) return;

    SDL_Color sdlColor = {color.r, color.g, color.b, color.a};
    SDL_Surface* surface = TTF_RenderUTF8_Blended(ttf, text.c_str(), sdlColor);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dst;
    dst.x = static_cast<int>(position.x);
    dst.y = static_cast<int>(position.y);
    dst.w = surface->w;
    dst.h = surface->h;

    SDL_RenderCopy(m_renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

TTF_Font* SDLPlatform::getOrOpenFont(uint32_t fontId, unsigned int size) {
    auto it = m_fonts.find(fontId);
    if (it == m_fonts.end()) return nullptr;

    auto& fontData = it->second;
    auto sizeIt = fontData.sizedFonts.find(size);
    if (sizeIt != fontData.sizedFonts.end()) {
        return sizeIt->second;
    }

    // Open font at this size and cache it
    TTF_Font* font = TTF_OpenFont(fontData.path.c_str(), static_cast<int>(size));
    if (!font) {
        LOG_ERROR("Failed to open font at size " << size);
        return nullptr;
    }

    fontData.sizedFonts[size] = font;
    return font;
}

// =============================================================================
// Timing
// =============================================================================
float SDLPlatform::getDeltaTime() const {
    return m_deltaTime;
}

// =============================================================================
// Window info
// =============================================================================
Vec2i SDLPlatform::getWindowSize() const {
    int w = 0, h = 0;
    SDL_GetWindowSize(m_window, &w, &h);
    return {w, h};
}

// =============================================================================
// Input
// =============================================================================
bool SDLPlatform::isKeyPressed(KeyCode key) const {
    return m_currKeys[static_cast<size_t>(key)];
}

bool SDLPlatform::isKeyJustPressed(KeyCode key) const {
    auto i = static_cast<size_t>(key);
    return m_currKeys[i] && !m_prevKeys[i];
}

bool SDLPlatform::isKeyJustReleased(KeyCode key) const {
    auto i = static_cast<size_t>(key);
    return !m_currKeys[i] && m_prevKeys[i];
}

Vec2f SDLPlatform::getTouchOrMousePosition() const {
    return m_touchPos;
}

bool SDLPlatform::isTouchOrMouseDown() const {
    return m_touchDown;
}

// =============================================================================
// Audio
// =============================================================================
void SDLPlatform::playSound(SoundHandle sound) {
    auto it = m_sounds.find(sound.id);
    if (it == m_sounds.end() || !it->second) return;

    Mix_VolumeChunk(it->second, m_sfxVolume);
    int channel = Mix_PlayChannel(-1, it->second, 0);
    if (channel == -1) {
        LOG_DEBUG("No free audio channel for sound");
    }
}

void SDLPlatform::playMusic(const std::string& path, bool loop) {
    if (m_music) {
        Mix_FreeMusic(m_music);
        m_music = nullptr;
    }

    m_music = Mix_LoadMUS(path.c_str());
    if (!m_music) {
        LOG_ERROR("Failed to load music: " << Mix_GetError());
        return;
    }

    Mix_PlayMusic(m_music, loop ? -1 : 1);
}

void SDLPlatform::stopMusic() {
    Mix_HaltMusic();
}

void SDLPlatform::setMusicVolume(float volume) {
    int vol = static_cast<int>(Math::clamp(volume, 0.0f, 1.0f) * MIX_MAX_VOLUME);
    Mix_VolumeMusic(vol);
}

void SDLPlatform::setSfxVolume(float volume) {
    m_sfxVolume = static_cast<int>(Math::clamp(volume, 0.0f, 1.0f) * MIX_MAX_VOLUME);
}

// =============================================================================
// Resource loading
// =============================================================================
TextureHandle SDLPlatform::loadTexture(const std::string& key,
                                         const std::string& path) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        LOG_ERROR("Failed to load texture '" << key << "': " << IMG_GetError());
        return TextureHandle{0};
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    int w = surface->w;
    int h = surface->h;
    SDL_FreeSurface(surface);

    if (!texture) {
        LOG_ERROR("Failed to create texture from surface: " << SDL_GetError());
        return TextureHandle{0};
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    uint32_t id = m_nextTexId++;
    m_textures[id] = {texture, w, h};
    LOG_DEBUG("Loaded texture '" << key << "' as handle " << id);
    return TextureHandle{id};
}

SoundHandle SDLPlatform::loadSound(const std::string& key,
                                     const std::string& path) {
    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (!chunk) {
        LOG_ERROR("Failed to load sound '" << key << "': " << Mix_GetError());
        return SoundHandle{0};
    }

    uint32_t id = m_nextSndId++;
    m_sounds[id] = chunk;
    LOG_DEBUG("Loaded sound '" << key << "' as handle " << id);
    return SoundHandle{id};
}

FontHandle SDLPlatform::loadFont(const std::string& key,
                                   const std::string& path) {
    // For SDL_ttf, we store the path and open at each requested size on demand
    // Verify the file is accessible by opening at a default size
    TTF_Font* test = TTF_OpenFont(path.c_str(), 16);
    if (!test) {
        LOG_ERROR("Failed to load font '" << key << "': " << TTF_GetError());
        return FontHandle{0};
    }

    uint32_t id = m_nextFontId++;
    FontData data;
    data.path = path;
    data.sizedFonts[16] = test; // cache the test-opened font
    m_fonts[id] = std::move(data);
    LOG_DEBUG("Loaded font '" << key << "' as handle " << id);
    return FontHandle{id};
}

// =============================================================================
// Resource unloading
// =============================================================================
void SDLPlatform::unloadTexture(TextureHandle handle) {
    auto it = m_textures.find(handle.id);
    if (it != m_textures.end()) {
        if (it->second.texture) SDL_DestroyTexture(it->second.texture);
        m_textures.erase(it);
    }
}

void SDLPlatform::unloadSound(SoundHandle handle) {
    auto it = m_sounds.find(handle.id);
    if (it != m_sounds.end()) {
        if (it->second) Mix_FreeChunk(it->second);
        m_sounds.erase(it);
    }
}

void SDLPlatform::unloadFont(FontHandle handle) {
    auto it = m_fonts.find(handle.id);
    if (it != m_fonts.end()) {
        for (auto& [size, font] : it->second.sizedFonts) {
            if (font) TTF_CloseFont(font);
        }
        m_fonts.erase(it);
    }
}

// =============================================================================
// Camera
// =============================================================================
void SDLPlatform::setCameraOffset(const Vec2f& offset) {
    m_cameraOffset = offset;
}

Vec2f SDLPlatform::getCameraOffset() const {
    return m_cameraOffset;
}

#endif // MARIO_WASM
