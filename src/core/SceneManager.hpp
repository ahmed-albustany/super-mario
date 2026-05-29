#pragma once

#include <memory>
#include <vector>
#include <variant>
#include "scenes/IScene.hpp"

class IPlatform;
class InputManager;

/// @brief Scene stack manager with deferred push/pop/replace.
///        Commands are queued during update and applied at end-of-frame,
///        preventing mid-update stack invalidation.
class SceneManager {
public:
    SceneManager() = default;

    // Non-copyable
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    /// @brief Queue a push — the new scene goes on top of the stack.
    void push(std::unique_ptr<IScene> scene);

    /// @brief Queue a replace — the top scene is swapped out.
    void replace(std::unique_ptr<IScene> scene);

    /// @brief Queue a pop — removes the top scene, revealing the one below.
    void pop();

    /// @brief Process input on the top scene only.
    void handleInput(const InputManager& input);

    /// @brief Update the top scene only.
    void update(float dt);

    /// @brief Render all scenes bottom-up (transparent scenes show those below).
    void render(IPlatform& platform);

    /// @brief Apply all queued push/pop/replace commands. Call once per frame after update.
    void applyPendingCommands();

    /// @brief Is the scene stack empty?
    [[nodiscard]] bool isEmpty() const;

    /// @brief Get the current top scene (may be null if stack is empty).
    [[nodiscard]] IScene* current() const;

    /// @brief Number of scenes on the stack.
    [[nodiscard]] size_t size() const;

private:
    // Deferred command types
    struct PushCmd    { std::unique_ptr<IScene> scene; };
    struct PopCmd     {};
    struct ReplaceCmd { std::unique_ptr<IScene> scene; };
    using Command = std::variant<PushCmd, PopCmd, ReplaceCmd>;

    std::vector<std::unique_ptr<IScene>> m_stack;
    std::vector<Command> m_pending;
};
