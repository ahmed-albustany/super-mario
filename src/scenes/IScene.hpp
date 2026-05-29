#pragma once

#include <string>

class IPlatform;
class InputManager;
class EventBus;

/// @brief Abstract base class for all game scenes.
///        Scenes form a stack — the topmost scene receives update(),
///        all scenes render bottom-up (so pause overlays work).
class IScene {
public:
    virtual ~IScene() = default;

    /// @brief Called when this scene becomes the top scene (pushed or revealed).
    virtual void onEnter() = 0;

    /// @brief Called when this scene is popped or replaced.
    virtual void onExit() = 0;

    /// @brief Process input for this scene.
    virtual void handleInput(const InputManager& input) = 0;

    /// @brief Fixed-timestep logic update.
    /// @param dt Fixed delta time (Config::FIXED_TIMESTEP).
    virtual void update(float dt) = 0;

    /// @brief Render this scene.
    /// @param platform The platform layer to draw through.
    virtual void render(IPlatform& platform) = 0;

    /// @brief If true, the scene below this one should still be rendered.
    ///        Override to true for overlay scenes (pause, HUD).
    [[nodiscard]] virtual bool isTransparent() const { return false; }

    /// @brief Debug name for logging.
    [[nodiscard]] virtual std::string name() const = 0;
};
