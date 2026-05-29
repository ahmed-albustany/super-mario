#pragma once

#include "scenes/IScene.hpp"
#include <string>

class Game;

/// @brief Asset preloading scene — reads manifest.json, loads all assets,
///        shows a progress bar, then transitions to MenuScene.
class BootScene final : public IScene {
public:
    explicit BootScene(Game& game);

    void onEnter() override;
    void onExit() override;
    void handleInput(const InputManager& input) override;
    void update(float dt) override;
    void render(IPlatform& platform) override;
    [[nodiscard]] std::string name() const override { return "BootScene"; }

private:
    Game& m_game;

    // Asset loading state
    struct AssetEntry {
        std::string type; // "texture", "sound", "font"
        std::string key;
        std::string path;
    };

    std::vector<AssetEntry> m_assets;
    size_t m_loadIndex  = 0;
    bool   m_loadDone   = false;
    int    m_failures   = 0;
    float  m_minDisplayTime = 0.5f; // minimum time to show loading screen
    float  m_elapsed    = 0.0f;
};
