#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include "platform/IPlatform.hpp"

/// @brief Singleton resource manager — maps string keys to platform handles.
///        All loading goes through IPlatform; this class tracks what's loaded
///        and provides key→handle lookup for game code.
///
///        Returns std::optional — caller MUST check. No silent nulls.
class ResourceManager {
public:
    /// @brief Get the singleton instance.
    static ResourceManager& instance() {
        static ResourceManager rm;
        return rm;
    }

    // Non-copyable
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    /// @brief Bind to a platform. Must be called once before any load/get calls.
    void init(IPlatform& platform) {
        m_platform = &platform;
    }

    // ---- Loading (delegates to platform) ----

    /// @brief Load a texture from file. Returns true if successful.
    bool loadTexture(const std::string& key, const std::string& path) {
        if (!m_platform) return false;
        if (m_textures.count(key)) return true; // already loaded
        auto handle = m_platform->loadTexture(key, path);
        if (!handle.valid()) return false;
        m_textures[key] = handle;
        return true;
    }

    /// @brief Load a sound effect from file. Returns true if successful.
    bool loadSound(const std::string& key, const std::string& path) {
        if (!m_platform) return false;
        if (m_sounds.count(key)) return true;
        auto handle = m_platform->loadSound(key, path);
        if (!handle.valid()) return false;
        m_sounds[key] = handle;
        return true;
    }

    /// @brief Load a font from file. Returns true if successful.
    bool loadFont(const std::string& key, const std::string& path) {
        if (!m_platform) return false;
        if (m_fonts.count(key)) return true;
        auto handle = m_platform->loadFont(key, path);
        if (!handle.valid()) return false;
        m_fonts[key] = handle;
        return true;
    }

    // ---- Getters (return optional handles) ----

    [[nodiscard]] std::optional<TextureHandle> getTexture(const std::string& key) const {
        auto it = m_textures.find(key);
        if (it == m_textures.end()) return std::nullopt;
        return it->second;
    }

    [[nodiscard]] std::optional<SoundHandle> getSound(const std::string& key) const {
        auto it = m_sounds.find(key);
        if (it == m_sounds.end()) return std::nullopt;
        return it->second;
    }

    [[nodiscard]] std::optional<FontHandle> getFont(const std::string& key) const {
        auto it = m_fonts.find(key);
        if (it == m_fonts.end()) return std::nullopt;
        return it->second;
    }

    // ---- Bulk loading ----

    /// @brief Preload all assets listed in a JSON manifest file.
    ///        Manifest format:
    ///        {
    ///          "textures": { "player": "textures/player.png", ... },
    ///          "sounds":   { "jump": "audio/jump.ogg", ... },
    ///          "fonts":    { "main": "fonts/main.ttf", ... }
    ///        }
    /// @param manifestPath Path to the JSON manifest (relative to assets root).
    /// @param assetsRoot   Base directory prepended to all asset paths.
    /// @return Number of assets that failed to load.
    int preload(const std::string& manifestPath, const std::string& assetsRoot = "");

    // ---- Cleanup ----

    void unload(const std::string& key) {
        m_textures.erase(key);
        m_sounds.erase(key);
        m_fonts.erase(key);
    }

    void unloadAll() {
        m_textures.clear();
        m_sounds.clear();
        m_fonts.clear();
    }

    /// @brief Check if any asset with this key is loaded.
    [[nodiscard]] bool isLoaded(const std::string& key) const {
        return m_textures.count(key) || m_sounds.count(key) || m_fonts.count(key);
    }

private:
    ResourceManager() = default;

    IPlatform* m_platform = nullptr;
    std::unordered_map<std::string, TextureHandle> m_textures;
    std::unordered_map<std::string, SoundHandle>   m_sounds;
    std::unordered_map<std::string, FontHandle>     m_fonts;
};
